#!/usr/bin/env python3
"""
Download binaries.zip artifact from the Cygwin Build workflow
for a specified branch of github.com/ArduPilot/ardupilot.

Usage:
    ./fetch_cygwin_build.py ArduPilot-4.7
    ./fetch_cygwin_build.py master
    ./fetch_cygwin_build.py ArduPilot-4.7 -o /tmp/binaries.zip
"""

import argparse
import subprocess
import sys
import json
import os

REPO = "ArduPilot/ardupilot"
WORKFLOW_ID = 8391275  # Cygwin Build


def gh_api(endpoint):
    """Call GitHub API via gh CLI and return parsed JSON."""
    result = subprocess.run(
        ["gh", "api", endpoint],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"Error: gh api {endpoint}\n{result.stderr}", file=sys.stderr)
        sys.exit(1)
    return json.loads(result.stdout)


def main():
    parser = argparse.ArgumentParser(description="Download Cygwin Build binaries.zip for a branch")
    parser.add_argument("branch", help="Branch name (e.g. ArduPilot-4.7, master)")
    parser.add_argument("-o", "--output", default="binaries.zip", help="Output filename (default: binaries.zip)")
    args = parser.parse_args()

    # Find the latest successful Cygwin Build run for this branch
    print(f"Finding latest Cygwin Build for branch '{args.branch}'...")
    data = gh_api(
        f"repos/{REPO}/actions/workflows/{WORKFLOW_ID}/runs"
        f"?branch={args.branch}&status=success&per_page=1"
    )

    if not data.get("workflow_runs"):
        print(f"No successful Cygwin Build runs found for branch '{args.branch}'", file=sys.stderr)
        sys.exit(1)

    run = data["workflow_runs"][0]
    run_id = run["id"]
    print(f"Found run #{run['run_number']} ({run['created_at']}) - {run['html_url']}")

    # Find the binaries artifact
    artifacts = gh_api(f"repos/{REPO}/actions/runs/{run_id}/artifacts")
    binaries = None
    for a in artifacts.get("artifacts", []):
        if a["name"] == "binaries":
            binaries = a
            break

    if not binaries:
        print("No 'binaries' artifact found in this run", file=sys.stderr)
        sys.exit(1)

    if binaries.get("expired"):
        print("Artifact has expired and is no longer available", file=sys.stderr)
        sys.exit(1)

    size_mb = binaries["size_in_bytes"] / (1024 * 1024)
    print(f"Downloading binaries ({size_mb:.1f} MB)...")

    # Download zip via GitHub API (gh api writes binary to stdout)
    result = subprocess.run(
        ["gh", "api",
         f"repos/{REPO}/actions/artifacts/{binaries['id']}/zip"],
        capture_output=True
    )
    if result.returncode != 0:
        print(f"Download failed: {result.stderr.decode()}", file=sys.stderr)
        sys.exit(1)

    with open(args.output, "wb") as f:
        f.write(result.stdout)

    actual_size = os.path.getsize(args.output)
    print(f"Saved to {args.output} ({actual_size / (1024*1024):.1f} MB)")


if __name__ == "__main__":
    main()
