#!/usr/bin/env python3
"""
Commit the working-tree changes of a checkout to a GitHub branch through the
GraphQL createCommitOnBranch mutation.

GitHub signs commits it creates this way, so they show as verified and satisfy
"Require signed commits" branch protection. A commit signed locally with a key
GitHub does not know (such as one generated inside a workflow run) shows as
unverified instead.

The checkout's HEAD must match the remote branch head: the mutation is refused
if the branch moved in the meantime. File modes are not transmitted, so the
script refuses to commit executable files rather than silently dropping the
executable bit.

Usage:
    GH_TOKEN=... commit_via_api.py --repo OWNER/NAME --branch main \
        --message "chore: release version 1.2.3"

Prints the new commit's SHA on success. With --dry-run, prints the mutation
input (file contents elided) instead of sending it.
"""

import argparse
import base64
import json
import os
import subprocess
import sys
import urllib.error
import urllib.request

MUTATION = """
mutation($input: CreateCommitOnBranchInput!) {
  createCommitOnBranch(input: $input) {
    commit { oid url }
  }
}
"""


def git(*args):
    return subprocess.run(["git", *args], check=True, capture_output=True).stdout


def collect_changes():
    """Return (additions, deletions) for every change relative to HEAD."""
    git("add", "-A")
    # -z keeps unusual file names intact; --no-renames reports a rename as a
    # deletion plus an addition, which is what the mutation expects.
    fields = git("diff", "--cached", "--name-status", "--no-renames", "-z", "HEAD")
    fields = fields.decode().split("\0")
    additions, deletions = [], []
    for status, path in zip(fields[0::2], fields[1::2]):
        if status == "D":
            deletions.append({"path": path})
            continue
        if os.access(path, os.X_OK):
            sys.exit(f"error: {path} is executable; createCommitOnBranch "
                     "cannot preserve file modes")
        with open(path, "rb") as handle:
            contents = base64.b64encode(handle.read()).decode()
        additions.append({"path": path, "contents": contents})
    return additions, deletions


def build_input(repo, branch, message, head, additions, deletions):
    headline, _, body = message.partition("\n")
    commit_message = {"headline": headline}
    if body.strip():
        commit_message["body"] = body.strip()
    return {
        "branch": {"repositoryNameWithOwner": repo, "branchName": branch},
        "message": commit_message,
        "expectedHeadOid": head,
        "fileChanges": {"additions": additions, "deletions": deletions},
    }


def send(api_url, token, mutation_input):
    request = urllib.request.Request(
        api_url,
        data=json.dumps({"query": MUTATION,
                         "variables": {"input": mutation_input}}).encode(),
        headers={"Authorization": f"bearer {token}",
                 "Content-Type": "application/json"},
    )
    try:
        with urllib.request.urlopen(request) as response:
            result = json.load(response)
    except urllib.error.HTTPError as error:
        sys.exit(f"error: GraphQL request failed with HTTP {error.code}: "
                 f"{error.read().decode(errors='replace')}")
    if result.get("errors"):
        sys.exit(f"error: createCommitOnBranch failed: {result['errors']}")
    return result["data"]["createCommitOnBranch"]["commit"]["oid"]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--repo", required=True, help="OWNER/NAME")
    parser.add_argument("--branch", required=True)
    parser.add_argument("--message", required=True)
    parser.add_argument("--api-url", default=os.environ.get(
        "GITHUB_GRAPHQL_URL", "https://api.github.com/graphql"))
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    head = git("rev-parse", "HEAD").decode().strip()
    additions, deletions = collect_changes()
    if not additions and not deletions:
        sys.exit("error: no changes to commit")
    mutation_input = build_input(args.repo, args.branch, args.message, head,
                                 additions, deletions)

    if args.dry_run:
        for addition in mutation_input["fileChanges"]["additions"]:
            addition["contents"] = f"<{len(addition['contents'])} base64 chars>"
        print(json.dumps(mutation_input, indent=2))
        return

    token = os.environ.get("GH_TOKEN")
    if not token:
        sys.exit("error: GH_TOKEN is not set")
    print(send(args.api_url, token, mutation_input))


if __name__ == "__main__":
    main()
