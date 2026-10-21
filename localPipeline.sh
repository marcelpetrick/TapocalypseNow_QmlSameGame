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
#   4. Configure a dedicated coverage build and generate an HTML report
#   5. Generate Doxygen HTML documentation and verify the warning log
#   6. Try to open the generated coverage and Doxygen reports automatically
#   7. Run discovered Qt qmllint build targets when they exist
#   8. Run clang-format on C++ sources when clang-format is installed
#   9. Run qmlformat on QML sources when qmlformat is installed
#   10. Print a stage-by-stage summary
#   11. Launch the built tapocalypse binary by default as the final interactive step
#
# Invocation:
#   ./localPipeline.sh
#   ./localPipeline.sh --verbose
#   ./localPipeline.sh --noRun
#   ./localPipeline.sh --noRun --noOpen    # CI / agents: no app, no browser
#   ./localPipeline.sh --build-dir build-debug --build-type Debug
#   ./localPipeline.sh --coverage-build-dir build-coverage
#   ./localPipeline.sh --generator Ninja --cmake-arg -DCMAKE_PREFIX_PATH=/path/to/Qt
#   TAPOCALYPSE_PIPELINE_JOBS=8 ./localPipeline.sh
#   TAPOCALYPSE_COVERAGE_MIN_LINES=95 ./localPipeline.sh   # coverage gate (default 90)
# -----------------------------------------------------------------------------

set -u
set -o pipefail

readonly SCRIPT_NAME="$(basename "$0")"
readonly PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly DEFAULT_BUILD_DIR="${PROJECT_ROOT}/build"
readonly DEFAULT_COVERAGE_BUILD_DIR="${PROJECT_ROOT}/build-coverage"
readonly DEFAULT_DOCS_BUILD_DIR="${PROJECT_ROOT}/build-docs"
readonly COVERAGE_MIN_LINES="${TAPOCALYPSE_COVERAGE_MIN_LINES:-90}"
readonly STAGE_LOG_DIR="${PROJECT_ROOT}/.localPipeline"

VERBOSE=0
NO_RUN=0
NO_OPEN=0
BUILD_DIR="${DEFAULT_BUILD_DIR}"
COVERAGE_BUILD_DIR="${DEFAULT_COVERAGE_BUILD_DIR}"
DOCS_BUILD_DIR="${DEFAULT_DOCS_BUILD_DIR}"
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
GCOVR_COMMAND=""
GCOV_COMMAND=""
PYTHON3_COMMAND=""
DOXYGEN_COMMAND=""

BUILD_OK=0
TESTS_OK=0
COVERAGE_OK=0
OPEN_COVERAGE_OK=0
DOXYGEN_OK=0
DOXYGEN_WARNINGS_OK=0
OPEN_DOCS_OK=0
QMLLINT_OK=0
CPP_FORMAT_OK=0
CPP_FORMAT_CHANGED=0
QML_FORMAT_OK=0
QML_FORMAT_CHANGED=0
COVERAGE_LINE_PERCENT=""

print_usage() {
    cat <<EOF
Usage: ${SCRIPT_NAME} [options]

Options:
  --verbose               Print commands and stream tool output
  --noRun                 Do not launch the game after the pipeline
  --noOpen                Do not hand coverage and Doxygen reports to the desktop opener
  --build-dir PATH        Build directory to configure into (default: ${DEFAULT_BUILD_DIR})
  --coverage-build-dir PATH
                          Coverage build directory (default: ${DEFAULT_COVERAGE_BUILD_DIR})
  --docs-build-dir PATH   Doxygen output directory (default: ${DEFAULT_DOCS_BUILD_DIR})
  --build-type TYPE       CMAKE_BUILD_TYPE value (default: ${BUILD_TYPE})
  --generator NAME        CMake generator name, for example "Ninja"
  --cmake-arg ARG         Extra argument passed to the CMake configure command
  --help, -h              Show this help text

Environment:
  TAPOCALYPSE_PIPELINE_JOBS  Positive integer override for parallel build/test jobs

Examples:
  ./localPipeline.sh
  ./localPipeline.sh --noRun --build-type Release
  ./localPipeline.sh --docs-build-dir build-docs
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

detect_open_command() {
    if command -v xdg-open >/dev/null 2>&1; then
        printf '%s\n' "xdg-open"
        return 0
    fi

    if command -v open >/dev/null 2>&1; then
        printf '%s\n' "open"
        return 0
    fi

    return 1
}

extract_summary_json_number() {
    local json_file="$1"
    local key="$2"

    "${PYTHON3_COMMAND}" - "${json_file}" "${key}" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as handle:
    payload = json.load(handle)

value = payload.get(sys.argv[2])
if value is None:
    raise SystemExit(1)

print(value)
PY
}

render_coverage_html() {
    local summary_json="$1"
    local line_report="$2"
    local branch_report="$3"
    local output_html="$4"

    "${PYTHON3_COMMAND}" - "${summary_json}" "${line_report}" "${branch_report}" "${output_html}" <<'PY'
import html
import json
import pathlib
import sys

summary_path = pathlib.Path(sys.argv[1])
line_report_path = pathlib.Path(sys.argv[2])
branch_report_path = pathlib.Path(sys.argv[3])
output_path = pathlib.Path(sys.argv[4])

summary = json.loads(summary_path.read_text(encoding="utf-8"))
line_report = line_report_path.read_text(encoding="utf-8")
branch_report = branch_report_path.read_text(encoding="utf-8")

files = summary.get("files", [])

def metric_cell(value: float) -> str:
    if value >= 99.9:
        color = "#0b6e4f"
    elif value >= 95.0:
        color = "#8a6d1b"
    else:
        color = "#8c2f39"
    return f'<td style="text-align:right;color:{color};font-weight:700">{value:.1f}%</td>'

rows = []
for entry in files:
    rows.append(
        "<tr>"
        f"<td>{html.escape(entry['filename'])}</td>"
        f"{metric_cell(float(entry['line_percent']))}"
        f"{metric_cell(float(entry['function_percent']))}"
        f"{metric_cell(float(entry['branch_percent']))}"
        "</tr>"
    )

output_path.write_text(
    f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Tapocalypse Coverage</title>
  <style>
    :root {{
      color-scheme: light;
      --bg: #f6f1e8;
      --panel: #fffaf2;
      --ink: #1f2933;
      --muted: #52606d;
      --accent: #b6412c;
      --border: #d9cbb8;
    }}
    body {{
      margin: 0;
      font-family: "Iosevka Etoile", "IBM Plex Sans", sans-serif;
      color: var(--ink);
      background:
        radial-gradient(circle at top left, rgba(182,65,44,0.16), transparent 28%),
        linear-gradient(180deg, #fdf9f1 0%, var(--bg) 100%);
    }}
    main {{
      max-width: 1100px;
      margin: 0 auto;
      padding: 40px 24px 56px;
    }}
    h1, h2 {{
      font-family: "Space Grotesk", "Avenir Next", sans-serif;
      letter-spacing: 0.02em;
      margin: 0 0 12px;
    }}
    p {{
      color: var(--muted);
      margin: 0 0 20px;
    }}
    .summary {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      gap: 16px;
      margin: 28px 0 32px;
    }}
    .card, .panel {{
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 18px;
      box-shadow: 0 14px 30px rgba(31, 41, 51, 0.06);
    }}
    .card {{
      padding: 18px 20px;
    }}
    .label {{
      color: var(--muted);
      font-size: 0.9rem;
      margin-bottom: 8px;
    }}
    .value {{
      font-size: 2rem;
      font-weight: 800;
      color: var(--accent);
    }}
    .panel {{
      padding: 20px 22px;
      margin-bottom: 22px;
    }}
    table {{
      width: 100%;
      border-collapse: collapse;
    }}
    th, td {{
      padding: 10px 12px;
      border-bottom: 1px solid var(--border);
    }}
    th {{
      text-align: left;
      color: var(--muted);
      font-size: 0.92rem;
    }}
    pre {{
      overflow-x: auto;
      white-space: pre-wrap;
      word-break: break-word;
      margin: 0;
      font-family: "Iosevka", "SFMono-Regular", monospace;
      font-size: 0.92rem;
      line-height: 1.45;
    }}
  </style>
</head>
<body>
  <main>
    <h1>Tapocalypse Coverage</h1>
    <p>Generated by the local pipeline from the dedicated coverage build.</p>

    <section class="summary">
      <div class="card">
        <div class="label">Line Coverage</div>
        <div class="value">{float(summary['line_percent']):.1f}%</div>
      </div>
      <div class="card">
        <div class="label">Function Coverage</div>
        <div class="value">{float(summary['function_percent']):.1f}%</div>
      </div>
      <div class="card">
        <div class="label">Branch Coverage</div>
        <div class="value">{float(summary['branch_percent']):.1f}%</div>
      </div>
    </section>

    <section class="panel">
      <h2>Per-File Summary</h2>
      <table>
        <thead>
          <tr>
            <th>File</th>
            <th>Lines</th>
            <th>Functions</th>
            <th>Branches</th>
          </tr>
        </thead>
        <tbody>
          {''.join(rows)}
        </tbody>
      </table>
    </section>

    <section class="panel">
      <h2>Line Report</h2>
      <pre>{html.escape(line_report)}</pre>
    </section>

    <section class="panel">
      <h2>Branch Report</h2>
      <pre>{html.escape(branch_report)}</pre>
    </section>
  </main>
</body>
</html>
""",
        encoding="utf-8",
    )
PY
}

render_doxygen_config() {
    local template_file="$1"
    local output_dir="$2"
    local config_file="$3"

    "${PYTHON3_COMMAND}" - "${template_file}" "${PROJECT_ROOT}" "${output_dir}" "${config_file}" <<'PY'
import pathlib
import sys

template_path = pathlib.Path(sys.argv[1])
project_root = pathlib.Path(sys.argv[2]).resolve()
output_dir = pathlib.Path(sys.argv[3]).resolve()
config_path = pathlib.Path(sys.argv[4]).resolve()

content = template_path.read_text(encoding="utf-8")
replacements = {
    "@PROJECT_NAME@": "Tapocalypse",
    "@PROJECT_VERSION@": "",
    "@PROJECT_ROOT@": str(project_root),
    "@DOXYGEN_OUTPUT_DIRECTORY@": str(output_dir),
}

for key, value in replacements.items():
    content = content.replace(key, value)

config_path.parent.mkdir(parents=True, exist_ok=True)
config_path.write_text(content, encoding="utf-8")
PY
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

configure_coverage_project() {
    local -a cmd=(cmake -S "${PROJECT_ROOT}" -B "${COVERAGE_BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug -DTAPOCALYPSE_ENABLE_COVERAGE=ON)

    if [[ -n "${GENERATOR}" ]]; then
        cmd+=(-G "${GENERATOR}")
    fi

    if [[ "${#CMAKE_CONFIGURE_ARGS[@]}" -gt 0 ]]; then
        cmd+=("${CMAKE_CONFIGURE_ARGS[@]}")
    fi

    log "Configuring dedicated coverage build in '${COVERAGE_BUILD_DIR}'."
    mkdir -p "${COVERAGE_BUILD_DIR}" || return 1

    run_command "Running CMake coverage configure step" "${cmd[@]}"
}

build_coverage_project() {
    local -a clean_first=()

    # gcov notes are compiler-version specific, and notes of objects that are no longer built are never refreshed.
    # When the configured compiler changes, drop only gcov artifacts and rebuild from clean.
    local compiler_stamp="${COVERAGE_BUILD_DIR}/.tapocalypse-compiler"
    local compiler_path compiler_id
    compiler_path="$(sed -n 's/^CMAKE_CXX_COMPILER:[A-Z]*=//p' "${COVERAGE_BUILD_DIR}/CMakeCache.txt" 2>/dev/null)"
    compiler_id="${compiler_path} $("${compiler_path:-c++}" --version 2>/dev/null | head -n 1)"
    if [[ "$(cat "${compiler_stamp}" 2>/dev/null)" != "${compiler_id}" ]]; then
        log "Coverage compiler changed or unknown; removing gcov data and rebuilding from clean."
        find "${COVERAGE_BUILD_DIR}" \( -name '*.gcno' -o -name '*.gcda' \) -type f -delete
        clean_first=(--clean-first)
    fi

    log "Building coverage configuration with ${PARALLEL_JOBS} parallel job(s)."
    run_command "Running coverage build" cmake --build "${COVERAGE_BUILD_DIR}" --parallel "${PARALLEL_JOBS}" "${clean_first[@]}" || return 1
    printf '%s\n' "${compiler_id}" > "${compiler_stamp}"
}

generate_doxygen_docs() {
    local docs_output_dir="${DOCS_BUILD_DIR}/doxygen"
    local docs_index_file="${docs_output_dir}/html/index.html"
    local docs_warning_file="${docs_output_dir}/warnings.txt"
    local docs_config_file="${STAGE_LOG_DIR}/Doxyfile.generated"
    local open_command=""

    log "Generating Doxygen documentation."

    mkdir -p "${STAGE_LOG_DIR}" "${docs_output_dir}" || {
        mark_result "Doxygen" "FAIL" "Could not create Doxygen output directories"
        mark_result "Doxygen Warn" "SKIP" "Skipped because Doxygen generation failed"
        mark_result "Open Docs" "SKIP" "Skipped because Doxygen generation failed"
        return 1
    }

    if ! render_doxygen_config "${PROJECT_ROOT}/Doxyfile.in" "${docs_output_dir}" "${docs_config_file}"; then
        mark_result "Doxygen" "FAIL" "Could not render Doxygen configuration"
        mark_result "Doxygen Warn" "SKIP" "Skipped because Doxygen generation failed"
        mark_result "Open Docs" "SKIP" "Skipped because Doxygen generation failed"
        return 1
    fi

    if ! run_command "Running Doxygen" "${DOXYGEN_COMMAND}" "${docs_config_file}"; then
        mark_result "Doxygen" "FAIL" "Doxygen execution failed"
        mark_result "Doxygen Warn" "SKIP" "Skipped because Doxygen generation failed"
        mark_result "Open Docs" "SKIP" "Skipped because Doxygen generation failed"
        return 1
    fi

    if [[ ! -f "${docs_index_file}" ]]; then
        mark_result "Doxygen" "FAIL" "Missing ${docs_index_file}"
        mark_result "Doxygen Warn" "SKIP" "Skipped because Doxygen generation failed"
        mark_result "Open Docs" "SKIP" "Skipped because Doxygen generation failed"
        return 1
    fi

    DOXYGEN_OK=1
    mark_result "Doxygen" "PASS" "HTML documentation generated in ${docs_output_dir}"

    if [[ ! -f "${docs_warning_file}" ]]; then
        mark_result "Doxygen Warn" "FAIL" "Missing ${docs_warning_file}"
    elif [[ -s "${docs_warning_file}" ]]; then
        mark_result "Doxygen Warn" "WARN" "Warnings were reported in ${docs_warning_file}"
    else
        DOXYGEN_WARNINGS_OK=1
        mark_result "Doxygen Warn" "PASS" "No Doxygen warnings were reported"
    fi

    if [[ "${NO_OPEN}" -eq 1 ]]; then
        mark_result "Open Docs" "SKIP" "Suppressed by --noOpen"
    elif open_command="$(detect_open_command)"; then
        log "Opening Doxygen docs with '${open_command}'."
        if run_command "Opening Doxygen index.html" "${open_command}" "${docs_index_file}"; then
            OPEN_DOCS_OK=1
            mark_result "Open Docs" "PASS" "Doxygen index.html was handed to the desktop opener"
        else
            mark_result "Open Docs" "WARN" "Could not open Doxygen index.html automatically"
        fi
    else
        mark_result "Open Docs" "SKIP" "No supported desktop opener was found"
    fi

    return 0
}

generate_coverage_report() {
    local coverage_output_dir="${COVERAGE_BUILD_DIR}/coverage"
    local coverage_html_dir="${coverage_output_dir}/html"
    local coverage_index_file="${coverage_html_dir}/index.html"
    local coverage_summary_file="${coverage_output_dir}/summary.json"
    local coverage_line_report_file="${coverage_output_dir}/coverage.txt"
    local coverage_branch_report_file="${coverage_output_dir}/branches.txt"
    local open_command=""
    local coverage_function_percent=""
    local coverage_branch_percent=""

    log "Generating coverage report."
    mkdir -p "${coverage_html_dir}" || {
        mark_result "Coverage" "FAIL" "Could not create coverage output directories"
        mark_result "Coverage Gate" "SKIP" "Skipped because coverage generation failed"
        mark_result "Open Coverage" "SKIP" "Skipped because coverage generation failed"
        return 1
    }

    # Counters from an earlier run (or an older compiler) do not match freshly
    # rebuilt notes files and make gcov abort, so always start from zero.
    find "${COVERAGE_BUILD_DIR}" -name '*.gcda' -type f -delete

    if ! run_command "Running coverage tests" "${CTEST_COMMAND}" --test-dir "${COVERAGE_BUILD_DIR}" --output-on-failure; then
        mark_result "Coverage" "FAIL" "Coverage tests failed"
        mark_result "Coverage Gate" "SKIP" "Skipped because coverage generation failed"
        mark_result "Open Coverage" "SKIP" "Skipped because coverage generation failed"
        return 1
    fi

    if ! run_command "Generating line coverage report" \
        "${GCOVR_COMMAND}" \
        --root "${PROJECT_ROOT}" \
        --filter '^src/' \
        --exclude '^src/main.cpp$' \
        --exclude-throw-branches \
        --exclude-unreachable-branches \
        --gcov-executable "${GCOV_COMMAND}" \
        --object-directory "${COVERAGE_BUILD_DIR}" \
        --txt "${coverage_line_report_file}"; then
        mark_result "Coverage" "FAIL" "Line coverage generation failed"
        mark_result "Coverage Gate" "SKIP" "Skipped because coverage generation failed"
        mark_result "Open Coverage" "SKIP" "Skipped because coverage generation failed"
        return 1
    fi

    if ! run_command "Generating branch coverage report" \
        "${GCOVR_COMMAND}" \
        --root "${PROJECT_ROOT}" \
        --filter '^src/' \
        --exclude '^src/main.cpp$' \
        --exclude-throw-branches \
        --exclude-unreachable-branches \
        --gcov-executable "${GCOV_COMMAND}" \
        --object-directory "${COVERAGE_BUILD_DIR}" \
        --txt-metric branch \
        --txt "${coverage_branch_report_file}"; then
        mark_result "Coverage" "FAIL" "Branch coverage generation failed"
        mark_result "Coverage Gate" "SKIP" "Skipped because coverage generation failed"
        mark_result "Open Coverage" "SKIP" "Skipped because coverage generation failed"
        return 1
    fi

    if ! run_command "Generating coverage summary" \
        "${GCOVR_COMMAND}" \
        --root "${PROJECT_ROOT}" \
        --filter '^src/' \
        --exclude '^src/main.cpp$' \
        --exclude-throw-branches \
        --exclude-unreachable-branches \
        --gcov-executable "${GCOV_COMMAND}" \
        --object-directory "${COVERAGE_BUILD_DIR}" \
        --json-summary-pretty \
        --json-summary "${coverage_summary_file}"; then
        mark_result "Coverage" "FAIL" "Coverage HTML generation failed"
        mark_result "Coverage Gate" "SKIP" "Skipped because coverage generation failed"
        mark_result "Open Coverage" "SKIP" "Skipped because coverage generation failed"
        return 1
    fi

    if [[ ! -f "${coverage_summary_file}" ]]; then
        mark_result "Coverage" "FAIL" "Missing ${coverage_summary_file}"
        mark_result "Coverage Gate" "SKIP" "Skipped because coverage generation failed"
        mark_result "Open Coverage" "SKIP" "Skipped because coverage generation failed"
        return 1
    fi

    COVERAGE_LINE_PERCENT="$(extract_summary_json_number "${coverage_summary_file}" "line_percent" 2>/dev/null || true)"
    coverage_function_percent="$(extract_summary_json_number "${coverage_summary_file}" "function_percent" 2>/dev/null || true)"
    coverage_branch_percent="$(extract_summary_json_number "${coverage_summary_file}" "branch_percent" 2>/dev/null || true)"
    if [[ -z "${COVERAGE_LINE_PERCENT}" || -z "${coverage_function_percent}" || -z "${coverage_branch_percent}" ]]; then
        mark_result "Coverage" "FAIL" "Could not parse summary metrics"
        mark_result "Coverage Gate" "SKIP" "Skipped because coverage generation failed"
        mark_result "Open Coverage" "SKIP" "Skipped because coverage generation failed"
        return 1
    fi

    if ! render_coverage_html "${coverage_summary_file}" "${coverage_line_report_file}" "${coverage_branch_report_file}" "${coverage_index_file}"; then
        mark_result "Coverage" "FAIL" "Coverage HTML generation failed"
        mark_result "Coverage Gate" "SKIP" "Skipped because coverage generation failed"
        mark_result "Open Coverage" "SKIP" "Skipped because coverage generation failed"
        return 1
    fi

    if [[ ! -f "${coverage_index_file}" ]]; then
        mark_result "Coverage" "FAIL" "Missing ${coverage_index_file}"
        mark_result "Coverage Gate" "SKIP" "Skipped because coverage generation failed"
        mark_result "Open Coverage" "SKIP" "Skipped because coverage generation failed"
        return 1
    fi

    log "Coverage HTML entry point: ${coverage_index_file}"
    log "Coverage totals: lines ${COVERAGE_LINE_PERCENT}%, functions ${coverage_function_percent}%, branches ${coverage_branch_percent}%"

    COVERAGE_OK=1
    mark_result "Coverage" "PASS" "Coverage HTML generated successfully in ${COVERAGE_BUILD_DIR}"
    if awk -v actual="${COVERAGE_LINE_PERCENT}" -v minimum="${COVERAGE_MIN_LINES}" 'BEGIN { exit !(actual + 0 >= minimum + 0) }'; then
        mark_result "Coverage Gate" "PASS" "Lines ${COVERAGE_LINE_PERCENT}% (min ${COVERAGE_MIN_LINES}%), functions ${coverage_function_percent}%, branches ${coverage_branch_percent}%"
    else
        mark_result "Coverage Gate" "FAIL" "Lines ${COVERAGE_LINE_PERCENT}% below the ${COVERAGE_MIN_LINES}% minimum"
    fi

    if [[ "${NO_OPEN}" -eq 1 ]]; then
        mark_result "Open Coverage" "SKIP" "Suppressed by --noOpen"
    elif open_command="$(detect_open_command)"; then
        log "Opening coverage report with '${open_command}'."
        if run_command "Opening coverage index.html" "${open_command}" "${coverage_index_file}"; then
            OPEN_COVERAGE_OK=1
            mark_result "Open Coverage" "PASS" "Coverage index.html was handed to the desktop opener"
        else
            mark_result "Open Coverage" "WARN" "Could not open coverage index.html automatically"
        fi
    else
        mark_result "Open Coverage" "SKIP" "No supported desktop opener was found"
    fi

    return 0
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
            --noOpen)
                NO_OPEN=1
                ;;
            --build-dir)
                shift
                if [[ "$#" -eq 0 || "$1" = -* ]]; then
                    error "Missing or invalid value for --build-dir."
                    exit 2
                fi
                BUILD_DIR="$1"
                ;;
            --coverage-build-dir)
                shift
                if [[ "$#" -eq 0 || "$1" = -* ]]; then
                    error "Missing or invalid value for --coverage-build-dir."
                    exit 2
                fi
                COVERAGE_BUILD_DIR="$1"
                ;;
            --docs-build-dir)
                shift
                if [[ "$#" -eq 0 || "$1" = -* ]]; then
                    error "Missing or invalid value for --docs-build-dir."
                    exit 2
                fi
                DOCS_BUILD_DIR="$1"
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
    log "Coverage build directory: ${COVERAGE_BUILD_DIR}"
    log "Docs build directory: ${DOCS_BUILD_DIR}"
    log "Build type: ${BUILD_TYPE}"
    if [[ -n "${GENERATOR}" ]]; then
        log "Generator: ${GENERATOR}"
    fi
    if [[ "${NO_RUN}" -eq 1 ]]; then
        log "Application launch is suppressed because --noRun was provided."
    fi

    require_command cmake || missing_prerequisites=1
    require_command gcovr || missing_prerequisites=1
    require_command gcov || missing_prerequisites=1
    require_command python3 || missing_prerequisites=1
    require_command doxygen || missing_prerequisites=1

    if ! CTEST_COMMAND="$(resolve_ctest_command)"; then
        warn "Could not resolve a working ctest executable. Test stage may be skipped."
        CTEST_COMMAND=""
    fi

    GCOVR_COMMAND="$(command -v gcovr 2>/dev/null || true)"
    GCOV_COMMAND="$(command -v gcov 2>/dev/null || true)"
    PYTHON3_COMMAND="$(command -v python3 2>/dev/null || true)"
    DOXYGEN_COMMAND="$(command -v doxygen 2>/dev/null || true)"

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
        if configure_coverage_project && build_coverage_project; then
            generate_coverage_report || true
        else
            mark_result "Coverage" "FAIL" "Coverage configure or build step failed"
            mark_result "Coverage Gate" "SKIP" "Skipped because coverage generation failed"
            mark_result "Open Coverage" "SKIP" "Skipped because coverage generation failed"
        fi
        generate_doxygen_docs || true
        if run_qmllint_targets_if_present; then
            :
        fi
        run_clang_format_if_present
        run_qmlformat_if_present
    else
        mark_result "CTest" "SKIP" "Skipped because build failed"
        mark_result "Coverage" "SKIP" "Skipped because build failed"
        mark_result "Coverage Gate" "SKIP" "Skipped because build failed"
        mark_result "Open Coverage" "SKIP" "Skipped because build failed"
        mark_result "Doxygen" "SKIP" "Skipped because build failed"
        mark_result "Doxygen Warn" "SKIP" "Skipped because build failed"
        mark_result "Open Docs" "SKIP" "Skipped because build failed"
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
