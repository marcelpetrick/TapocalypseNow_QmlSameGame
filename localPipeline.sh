#!/usr/bin/env bash

# -----------------------------------------------------------------------------
# localPipeline.sh
#
# Purpose:
#   Run a local developer pipeline for this Tapocalypse Qt/QML project:
#   configure, build, run any discovered project tooling, and then launch
#   the game as the final interactive step.
#
# What it does:
#   1. Configure the project with CMake
#   2. Build the default targets in parallel
#   3. Run CTest if the build tree contains tests
#   4. Run discovered Qt qmllint build targets when they exist
#   5. Run clang-format on C++ sources when clang-format is installed
#   6. Run qmlformat on QML sources when qmlformat is installed
#   7. Print a stage-by-stage summary
#   8. Launch the built tapocalypse binary by default as the final interactive step
#
# Invocation:
#   ./localPipeline.sh
#   ./localPipeline.sh --verbose
#   ./localPipeline.sh --noRun
#   ./localPipeline.sh --build-dir build-debug --build-type Debug
#   ./localPipeline.sh --generator Ninja --cmake-arg -DCMAKE_PREFIX_PATH=/path/to/Qt
#   TAPOCALYPSE_PIPELINE_JOBS=8 ./localPipeline.sh
# -----------------------------------------------------------------------------

set -u
set -o pipefail

readonly SCRIPT_NAME="$(basename "$0")"
readonly PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly DEFAULT_BUILD_DIR="${PROJECT_ROOT}/build"
readonly STAGE_LOG_DIR="${PROJECT_ROOT}/.localPipeline"

VERBOSE=0
NO_RUN=0
BUILD_DIR="${DEFAULT_BUILD_DIR}"
BUILD_TYPE="Debug"
GENERATOR=""

declare -a CMAKE_CONFIGURE_ARGS=()
declare -a SUMMARY_LINES=()
declare -a CPP_FORMAT_TARGETS=()
declare -a QML_FORMAT_TARGETS=()
declare -a CPP_FORMAT_CHANGED_FILES=()
declare -a QML_FORMAT_CHANGED_FILES=()
declare -a QMLLINT_TARGETS=()

PARALLEL_JOBS=1
CHECKSUM_COMMAND=""
CTEST_COMMAND=""

BUILD_OK=0
TESTS_OK=0
QMLLINT_OK=0
CPP_FORMAT_OK=0
CPP_FORMAT_CHANGED=0
QML_FORMAT_OK=0
QML_FORMAT_CHANGED=0

print_usage() {
    cat <<EOF
Usage: ${SCRIPT_NAME} [options]

Options:
  --verbose               Print commands and stream tool output
  --noRun                 Do not launch the game after the pipeline
  --build-dir PATH        Build directory to configure into (default: ${DEFAULT_BUILD_DIR})
  --build-type TYPE       CMAKE_BUILD_TYPE value (default: ${BUILD_TYPE})
  --generator NAME        CMake generator name, for example "Ninja"
  --cmake-arg ARG         Extra argument passed to the CMake configure command
  --help, -h              Show this help text

Environment:
  TAPOCALYPSE_PIPELINE_JOBS  Positive integer override for parallel build/test jobs

Examples:
  ./localPipeline.sh
  ./localPipeline.sh --noRun --build-type Release
  ./localPipeline.sh --generator Ninja --cmake-arg -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.0/gcc_64
EOF
}

log() {
    printf '[INFO] %s\n' "$*"
}

warn() {
    printf '[WARN] %s\n' "$*" >&2
}

error() {
    printf '[ERROR] %s\n' "$*" >&2
}

verbose_log() {
    if [[ "${VERBOSE}" -eq 1 ]]; then
        printf '[VERBOSE] %s\n' "$*"
    fi
}

add_summary() {
    SUMMARY_LINES+=("$1")
}

mark_result() {
    local label="$1"
    local status="$2"
    local details="$3"
    add_summary "$(printf '%-18s : %-4s %s' "${label}" "${status}" "${details}")"
}

run_command() {
    local description="$1"
    shift
    local log_file=""

    verbose_log "${description}"
    verbose_log "Command: $*"

    if [[ "${VERBOSE}" -eq 1 ]]; then
        "$@"
        return $?
    fi

    mkdir -p "${STAGE_LOG_DIR}" || {
        error "Could not create stage log directory: ${STAGE_LOG_DIR}"
        return 1
    }

    log_file="$(mktemp "${STAGE_LOG_DIR}/$(printf '%s' "${description}" | tr ' /' '__').XXXXXX.log")" || {
        error "Could not create a temporary log file for '${description}'."
        return 1
    }

    if "$@" >"${log_file}" 2>&1; then
        rm -f "${log_file}"
        return 0
    fi

    error "${description} failed. Captured output:"
    sed 's/^/  | /' "${log_file}" >&2
    return 1
}

require_command() {
    local command_name="$1"
    if ! command -v "${command_name}" >/dev/null 2>&1; then
        error "Required command '${command_name}' was not found in PATH."
        return 1
    fi
    return 0
}

detect_parallel_jobs() {
    local configured_jobs="${TAPOCALYPSE_PIPELINE_JOBS:-}"
    local detected_jobs=""

    if [[ -n "${configured_jobs}" ]]; then
        if [[ "${configured_jobs}" =~ ^[1-9][0-9]*$ ]]; then
            printf '%s\n' "${configured_jobs}"
            return 0
        fi

        error "TAPOCALYPSE_PIPELINE_JOBS must be a positive integer, got '${configured_jobs}'."
        return 1
    fi

    if command -v nproc >/dev/null 2>&1; then
        detected_jobs="$(nproc)"
    elif command -v getconf >/dev/null 2>&1; then
        detected_jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || true)"
    elif command -v sysctl >/dev/null 2>&1; then
        detected_jobs="$(sysctl -n hw.ncpu 2>/dev/null || true)"
    fi

    if [[ "${detected_jobs}" =~ ^[1-9][0-9]*$ ]]; then
        printf '%s\n' "${detected_jobs}"
        return 0
    fi

    printf '1\n'
    return 0
}

resolve_checksum_command() {
    if command -v sha256sum >/dev/null 2>&1; then
        printf 'sha256sum\n'
        return 0
    fi

    if command -v shasum >/dev/null 2>&1; then
        printf 'shasum -a 256\n'
        return 0
    fi

    return 1
}

resolve_ctest_command() {
    local cmake_path=""
    local cmake_dir=""
    local sibling_ctest=""

    cmake_path="$(command -v cmake 2>/dev/null)" || return 1
    cmake_dir="$(dirname "${cmake_path}")"
    sibling_ctest="${cmake_dir}/ctest"

    if [[ -x "${sibling_ctest}" ]]; then
        printf '%s\n' "${sibling_ctest}"
        return 0
    fi

    if command -v ctest >/dev/null 2>&1; then
        command -v ctest
        return 0
    fi

    return 1
}

collect_cpp_format_targets() {
    if [[ ! -d "${PROJECT_ROOT}/src" ]]; then
        return 0
    fi

    mapfile -t CPP_FORMAT_TARGETS < <(
        cd "${PROJECT_ROOT}" &&
            find src -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' -o -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o -name '*.hxx' \) | LC_ALL=C sort
    )
}

collect_qml_format_targets() {
    if [[ ! -d "${PROJECT_ROOT}/qml" ]]; then
        return 0
    fi

    mapfile -t QML_FORMAT_TARGETS < <(
        cd "${PROJECT_ROOT}" &&
            find qml -type f \( -name '*.qml' -o -name '*.js' -o -name '*.mjs' \) | LC_ALL=C sort
    )
}

checksum_files() {
    (
        cd "${PROJECT_ROOT}" || exit 1
        if [[ "${CHECKSUM_COMMAND}" == "sha256sum" ]]; then
            sha256sum "$@"
        else
            shasum -a 256 "$@"
        fi
    )
}

find_changed_files_from_checksums() {
    local before_file="$1"
    local after_file="$2"

    awk '
        NR == FNR {
            before[$2] = $1
            next
        }
        {
            if (!($2 in before) || before[$2] != $1) {
                print $2
            }
        }
    ' "${before_file}" "${after_file}"
}

capture_format_changes() {
    local tool_name="$1"
    local before_file="$2"
    local after_file="$3"
    local changed_array_name="$4"
    local changed_flag_name="$5"

    mapfile -t "${changed_array_name}" < <(find_changed_files_from_checksums "${before_file}" "${after_file}")

    if eval '[[ "${#'"${changed_array_name}"'[@]}" -gt 0 ]]'; then
        printf -v "${changed_flag_name}" '%s' "1"
        warn "${tool_name} changed one or more files."
        eval 'printf "%s\n" "${'"${changed_array_name}"'[@]}" | sed "s/^/  - /"'
    else
        log "${tool_name} left all target files unchanged."
    fi
}

configure_project() {
    local -a cmd=(cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}")

    if [[ -n "${GENERATOR}" ]]; then
        cmd+=(-G "${GENERATOR}")
    fi

    if [[ "${#CMAKE_CONFIGURE_ARGS[@]}" -gt 0 ]]; then
        cmd+=("${CMAKE_CONFIGURE_ARGS[@]}")
    fi

    log "Configuring project in '${BUILD_DIR}'."
    mkdir -p "${BUILD_DIR}" || return 1

    run_command "Running CMake configure step" "${cmd[@]}"
}

build_project() {
    log "Building project with ${PARALLEL_JOBS} parallel job(s)."
    run_command "Running project build" cmake --build "${BUILD_DIR}" --parallel "${PARALLEL_JOBS}"
}

discover_qmllint_targets() {
    local help_output=""
    local candidate=""

    help_output="$(cmake --build "${BUILD_DIR}" --target help 2>/dev/null || true)"
    if [[ -z "${help_output}" ]]; then
        return 0
    fi

    while IFS= read -r candidate; do
        case "${candidate}" in
            all_qmllint|qmllint|*_qmllint)
                QMLLINT_TARGETS+=("${candidate}")
                ;;
        esac
    done < <(printf '%s\n' "${help_output}" | sed -n 's/^[.][.][.] //p' | sed 's/ .*//')

    if [[ "${#QMLLINT_TARGETS[@]}" -gt 0 ]]; then
        mapfile -t QMLLINT_TARGETS < <(printf '%s\n' "${QMLLINT_TARGETS[@]}" | LC_ALL=C sort -u)
    fi
}

run_tests_if_present() {
    local test_list_output=""

    if [[ -z "${CTEST_COMMAND}" ]]; then
        mark_result "CTest" "SKIP" "ctest is not installed"
        return 0
    fi

    test_list_output="$("${CTEST_COMMAND}" --test-dir "${BUILD_DIR}" -N 2>/dev/null || true)"
    if [[ "${test_list_output}" != *"Total Tests:"* ]]; then
        mark_result "CTest" "SKIP" "No CTest metadata found in ${BUILD_DIR}"
        return 0
    fi

    if [[ "${test_list_output}" == *"Total Tests: 0"* ]]; then
        mark_result "CTest" "SKIP" "No tests are registered for this project"
        return 0
    fi

    log "Running CTest with ${PARALLEL_JOBS} parallel job(s)."
    if run_command "Executing CTest" "${CTEST_COMMAND}" --test-dir "${BUILD_DIR}" --output-on-failure --parallel "${PARALLEL_JOBS}"; then
        TESTS_OK=1
        mark_result "CTest" "PASS" "Registered tests completed without failures"
    else
        mark_result "CTest" "FAIL" "CTest reported at least one failure"
    fi
}

run_qmllint_targets_if_present() {
    local target=""

    discover_qmllint_targets
    if [[ "${#QMLLINT_TARGETS[@]}" -eq 0 ]]; then
        mark_result "QML Lint" "SKIP" "No qmllint build targets were discovered"
        return 0
    fi

    for target in "${QMLLINT_TARGETS[@]}"; do
        log "Running discovered QML lint target '${target}'."
        if ! run_command "Building ${target}" cmake --build "${BUILD_DIR}" --target "${target}" --parallel "${PARALLEL_JOBS}"; then
            mark_result "QML Lint" "FAIL" "Target '${target}' failed"
            return 1
        fi
    done

    QMLLINT_OK=1
    mark_result "QML Lint" "PASS" "Ran ${#QMLLINT_TARGETS[@]} discovered qmllint target(s)"
}

run_clang_format_if_present() {
    local before_checksums_file=""
    local after_checksums_file=""

    if [[ "${#CPP_FORMAT_TARGETS[@]}" -eq 0 ]]; then
        mark_result "clang-format" "SKIP" "No C/C++ source files were found under src/"
        return 0
    fi

    if ! command -v clang-format >/dev/null 2>&1; then
        mark_result "clang-format" "SKIP" "clang-format is not installed"
        return 0
    fi

    mkdir -p "${STAGE_LOG_DIR}" || {
        mark_result "clang-format" "FAIL" "Could not create ${STAGE_LOG_DIR}"
        return 1
    }

    before_checksums_file="$(mktemp "${STAGE_LOG_DIR}/clang-format.before.XXXXXX.txt")" || return 1
    after_checksums_file="$(mktemp "${STAGE_LOG_DIR}/clang-format.after.XXXXXX.txt")" || {
        rm -f "${before_checksums_file}"
        return 1
    }

    checksum_files "${CPP_FORMAT_TARGETS[@]}" >"${before_checksums_file}" || {
        rm -f "${before_checksums_file}" "${after_checksums_file}"
        mark_result "clang-format" "FAIL" "Could not capture pre-format checksums"
        return 1
    }

    log "Running clang-format on ${#CPP_FORMAT_TARGETS[@]} C/C++ file(s)."
    if ! (
        cd "${PROJECT_ROOT}" || exit 1
        clang-format -i "${CPP_FORMAT_TARGETS[@]}"
    ); then
        rm -f "${before_checksums_file}" "${after_checksums_file}"
        mark_result "clang-format" "FAIL" "clang-format execution failed"
        return 1
    fi

    checksum_files "${CPP_FORMAT_TARGETS[@]}" >"${after_checksums_file}" || {
        rm -f "${before_checksums_file}" "${after_checksums_file}"
        mark_result "clang-format" "FAIL" "Could not capture post-format checksums"
        return 1
    }

    capture_format_changes "clang-format" "${before_checksums_file}" "${after_checksums_file}" CPP_FORMAT_CHANGED_FILES CPP_FORMAT_CHANGED
    rm -f "${before_checksums_file}" "${after_checksums_file}"

    CPP_FORMAT_OK=1
    if [[ "${CPP_FORMAT_CHANGED}" -eq 1 ]]; then
        mark_result "clang-format" "WARN" "Formatting completed and changed files"
    else
        mark_result "clang-format" "PASS" "Formatting completed without changing files"
    fi
}

run_qmlformat_if_present() {
    local before_checksums_file=""
    local after_checksums_file=""

    if [[ "${#QML_FORMAT_TARGETS[@]}" -eq 0 ]]; then
        mark_result "qmlformat" "SKIP" "No QML or JS source files were found under qml/"
        return 0
    fi

    if ! command -v qmlformat >/dev/null 2>&1; then
        mark_result "qmlformat" "SKIP" "qmlformat is not installed"
        return 0
    fi

    mkdir -p "${STAGE_LOG_DIR}" || {
        mark_result "qmlformat" "FAIL" "Could not create ${STAGE_LOG_DIR}"
        return 1
    }

    before_checksums_file="$(mktemp "${STAGE_LOG_DIR}/qmlformat.before.XXXXXX.txt")" || return 1
    after_checksums_file="$(mktemp "${STAGE_LOG_DIR}/qmlformat.after.XXXXXX.txt")" || {
        rm -f "${before_checksums_file}"
        return 1
    }

    checksum_files "${QML_FORMAT_TARGETS[@]}" >"${before_checksums_file}" || {
        rm -f "${before_checksums_file}" "${after_checksums_file}"
        mark_result "qmlformat" "FAIL" "Could not capture pre-format checksums"
        return 1
    }

    log "Running qmlformat on ${#QML_FORMAT_TARGETS[@]} QML/JS file(s)."
    if ! (
        cd "${PROJECT_ROOT}" || exit 1
        qmlformat -i "${QML_FORMAT_TARGETS[@]}"
    ); then
        rm -f "${before_checksums_file}" "${after_checksums_file}"
        mark_result "qmlformat" "FAIL" "qmlformat execution failed"
        return 1
    fi

    checksum_files "${QML_FORMAT_TARGETS[@]}" >"${after_checksums_file}" || {
        rm -f "${before_checksums_file}" "${after_checksums_file}"
        mark_result "qmlformat" "FAIL" "Could not capture post-format checksums"
        return 1
    }

    capture_format_changes "qmlformat" "${before_checksums_file}" "${after_checksums_file}" QML_FORMAT_CHANGED_FILES QML_FORMAT_CHANGED
    rm -f "${before_checksums_file}" "${after_checksums_file}"

    QML_FORMAT_OK=1
    if [[ "${QML_FORMAT_CHANGED}" -eq 1 ]]; then
        mark_result "qmlformat" "WARN" "Formatting completed and changed files"
    else
        mark_result "qmlformat" "PASS" "Formatting completed without changing files"
    fi
}

find_application_path() {
    local candidate="${BUILD_DIR}/bin/tapocalypse"

    if [[ -x "${candidate}" ]]; then
        printf '%s\n' "${candidate}"
        return 0
    fi

    candidate="$(find "${BUILD_DIR}" -type f -perm -111 \( -name 'tapocalypse' -o -name 'tapocalypse.exe' \) | head -n 1)"
    if [[ -n "${candidate}" ]]; then
        printf '%s\n' "${candidate}"
        return 0
    fi

    return 1
}

launch_application() {
    local app_path=""

    if ! app_path="$(find_application_path)"; then
        mark_result "Launch App" "WARN" "Built game binary was not found below ${BUILD_DIR}"
        return 1
    fi

    log "Launching tapocalypse as the final pipeline step."
    log "Close the game window to let the script finish."

    if run_command "Launching the Tapocalypse application" "${app_path}"; then
        mark_result "Launch App" "PASS" "tapocalypse started and the script resumed after it exited"
        return 0
    fi

    mark_result "Launch App" "WARN" "Launching ${app_path} failed; this does not change the pipeline result"
    return 1
}

print_summary() {
    local line=""
    printf '\n========== Local Pipeline Summary ==========\n'
    for line in "${SUMMARY_LINES[@]}"; do
        printf '%s\n' "${line}"
    done
    printf '============================================\n'
}

parse_arguments() {
    while [[ "$#" -gt 0 ]]; do
        case "$1" in
            --verbose)
                VERBOSE=1
                ;;
            --noRun)
                NO_RUN=1
                ;;
            --build-dir)
                shift
                if [[ "$#" -eq 0 || "$1" = -* ]]; then
                    error "Missing or invalid value for --build-dir."
                    exit 2
                fi
                BUILD_DIR="$1"
                ;;
            --build-type)
                shift
                if [[ "$#" -eq 0 || "$1" = -* ]]; then
                    error "Missing or invalid value for --build-type."
                    exit 2
                fi
                BUILD_TYPE="$1"
                ;;
            --generator)
                shift
                if [[ "$#" -eq 0 || "$1" = -* ]]; then
                    error "Missing or invalid value for --generator."
                    exit 2
                fi
                GENERATOR="$1"
                ;;
            --cmake-arg)
                shift
                if [[ "$#" -eq 0 ]]; then
                    error "Missing value for --cmake-arg."
                    exit 2
                fi
                CMAKE_CONFIGURE_ARGS+=("$1")
                ;;
            --help|-h)
                print_usage
                exit 0
                ;;
            *)
                error "Unknown argument: $1"
                print_usage
                exit 2
                ;;
        esac
        shift
    done
}

main() {
    local missing_prerequisites=0
    local exit_code=1

    parse_arguments "$@"

    log "Project root: ${PROJECT_ROOT}"
    log "Build directory: ${BUILD_DIR}"
    log "Build type: ${BUILD_TYPE}"
    if [[ -n "${GENERATOR}" ]]; then
        log "Generator: ${GENERATOR}"
    fi
    if [[ "${NO_RUN}" -eq 1 ]]; then
        log "Application launch is suppressed because --noRun was provided."
    fi

    require_command cmake || missing_prerequisites=1

    if ! CTEST_COMMAND="$(resolve_ctest_command)"; then
        warn "Could not resolve a working ctest executable. Test stage may be skipped."
        CTEST_COMMAND=""
    fi

    if ! CHECKSUM_COMMAND="$(resolve_checksum_command)"; then
        error "Neither 'sha256sum' nor 'shasum' was found in PATH."
        missing_prerequisites=1
    fi

    if ! PARALLEL_JOBS="$(detect_parallel_jobs)"; then
        missing_prerequisites=1
    else
        log "Parallel jobs: ${PARALLEL_JOBS}"
    fi

    collect_cpp_format_targets
    collect_qml_format_targets

    if [[ "${missing_prerequisites}" -ne 0 ]]; then
        add_summary "Prerequisite check failed. See errors above."
        print_summary
        exit 1
    fi

    if configure_project && build_project; then
        BUILD_OK=1
        mark_result "Configure+Build" "PASS" "Project configured and built in ${BUILD_DIR}"
    else
        mark_result "Configure+Build" "FAIL" "Configure or build step failed"
    fi

    if [[ "${BUILD_OK}" -eq 1 ]]; then
        run_tests_if_present
        if run_qmllint_targets_if_present; then
            :
        fi
        run_clang_format_if_present
        run_qmlformat_if_present
    else
        mark_result "CTest" "SKIP" "Skipped because build failed"
        mark_result "QML Lint" "SKIP" "Skipped because build failed"
        mark_result "clang-format" "SKIP" "Skipped because build failed"
        mark_result "qmlformat" "SKIP" "Skipped because build failed"
    fi

    if [[ "${BUILD_OK}" -eq 1 ]]; then
        exit_code=0
        if printf '%s\n' "${SUMMARY_LINES[@]}" | grep -Eq ': FAIL '; then
            exit_code=1
        fi
    fi

    if [[ "${BUILD_OK}" -eq 1 ]]; then
        if [[ "${NO_RUN}" -eq 1 ]]; then
            mark_result "Launch App" "SKIP" "Suppressed by --noRun"
        else
            launch_application || true
        fi
    else
        mark_result "Launch App" "SKIP" "Skipped because build failed"
    fi

    print_summary
    exit "${exit_code}"
}

main "$@"
