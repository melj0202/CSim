---
name: csim-integrate-feature-branch
description: Safely integrate an explicitly named and verified CSim or Illumo feature branch through linked Windows worktrees. Use only when the user explicitly requests a merge, fast-forward, push, or completed feature integration. Do not infer Git mutation authorization from implementation, review, validation, or readiness requests.
---

# CSim Feature Branch Integration

Perform only the exact Git side effects the user requested. Treat fetch, merge, push, branch changes, and worktree changes as separate permissions.

## Confirm the integration contract

Identify the exact feature branch, target branch, relevant worktrees, and requested operations. Require clean involved worktrees or stop and describe the conflicting state. Do not infer integration authority from a request to implement, review, verify, or declare work ready.

## Inspect before mutation

1. Run status, branch, log, and worktree inspection in both feature and target worktrees.
2. Check matching local and remote-tracking refs before creating or switching a branch.
3. Establish merge-base, ahead/behind counts, target ancestry, the exact commit range, and the complete feature diff.
4. Confirm required build, test, documentation, and review evidence belongs to the commit being integrated.
5. Do not fetch or pull merely to make remote state current unless the user requested it.

## Prefer a verified fast-forward

When the target is an ancestor of the verified feature tip and integration is authorized, run in the target worktree:

```powershell
git merge --ff-only <feature-branch>
```

Do not manipulate shared Git metadata manually. If a linked-worktree ref lock needs elevation, request the narrowest command-scoped permission and recheck state afterward.

If fast-forward is impossible, stop unless the user authorized reconciliation. When authorized, reconcile the target into the writable feature worktree, preserve canonical documentation and policies, resolve and validate there, then fast-forward the target. Do not leave a primary worktree conflicted. Never rebase, force-push, reset, restore, or discard work without explicit authorization.

## Verify the result

Confirm target branch, exact tip hash, ancestry, worktree cleanliness, and relevant tests. If push was explicitly requested, push the exact target branch and then compare local target, remote-tracking target, and intended feature commit. Report separately what was committed, merged, fetched, pushed, or left local.
