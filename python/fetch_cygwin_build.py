#!/usr/bin/env python3
"""
Download binaries.zip artifact from the Cygwin Build workflow
for a specified branch of github.com/ArduPilot/ardupilot.

Requires a GitHub token (public repo read-only is sufficient), set via:
  - GITHUB_TOKEN environment variable, or
  - ~/.config/fetch_cygwin_token (single line with token), or
  - ~/.config/gh/hosts.yml (gh CLI config)

Usage:
    ./fetch_cygwin_build.py ArduPilot-4.7
    ./fetch_cygwin_build.py master
    ./fetch_cygwin_build.py ArduPilot-4.7 -o /tmp/binaries.zip
"""

import argparse
import sys
import os
try:
    import requests
except ImportError:
    print("Error: 'requests' module required. Install with: pip3 install requests", file=sys.stderr)
    sys.exit(1)

API = "https://api.github.com"
REPO = "ArduPilot/ardupilot"
WORKFLOW_ID = 8391275  # Cygwin Build


def get_token():
    """Get GitHub token from environment or gh CLI config."""
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        return token
    # Try dedicated token file
    token_file = os.path.expanduser("~/.config/fetch_cygwin_token")
    if os.path.exists(token_file):
        with open(token_file) as f:
            token = f.read().strip()
            if token:
                return token
    # Try gh CLI config (simple YAML parse without yaml module)
    gh_hosts = os.path.expanduser("~/.config/gh/hosts.yml")
    if os.path.exists(gh_hosts):
        with open(gh_hosts) as f:
            for line in f:
                line = line.strip()
                if line.startswith("oauth_token:"):
                    token = line.split(":", 1)[1].strip()
                    if token:
                        return token
    print("Error: No GitHub token found.", file=sys.stderr)
    print("Set GITHUB_TOKEN, or create ~/.config/fetch_cygwin_token", file=sys.stderr)
    sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description="Download Cygwin Build binaries.zip for a branch")
    parser.add_argument("branch", help="Branch name (e.g. ArduPilot-4.7, master)")
    parser.add_argument("-o", "--output", default="binaries.zip", help="Output filename (default: binaries.zip)")
    args = parser.parse_args()

    token = get_token()
    headers = {
        "Authorization": f"token {token}",
        "Accept": "application/vnd.github+json",
    }

    # Find the latest successful Cygwin Build run for this branch
    print(f"Finding latest Cygwin Build for branch '{args.branch}'...")
    r = requests.get(
        f"{API}/repos/{REPO}/actions/workflows/{WORKFLOW_ID}/runs",
        headers=headers,
        params={"branch": args.branch, "status": "success", "per_page": 1},
    )
    r.raise_for_status()
    data = r.json()

    if not data.get("workflow_runs"):
        print(f"No successful Cygwin Build runs found for branch '{args.branch}'", file=sys.stderr)
        sys.exit(1)

    run = data["workflow_runs"][0]
    run_id = run["id"]
    print(f"Found run #{run['run_number']} ({run['created_at']}) - {run['html_url']}")

    # Find the binaries artifact
    r = requests.get(f"{API}/repos/{REPO}/actions/runs/{run_id}/artifacts", headers=headers)
    r.raise_for_status()
    binaries = None
    for a in r.json().get("artifacts", []):
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

    # Download the artifact zip
    r = requests.get(
        f"{API}/repos/{REPO}/actions/artifacts/{binaries['id']}/zip",
        headers=headers,
        stream=True,
        allow_redirects=True,
    )
    r.raise_for_status()

    with open(args.output, "wb") as f:
        for chunk in r.iter_content(chunk_size=65536):
            f.write(chunk)

    actual_size = os.path.getsize(args.output)
    print(f"Saved to {args.output} ({actual_size / (1024*1024):.1f} MB)")


if __name__ == "__main__":
    main()
