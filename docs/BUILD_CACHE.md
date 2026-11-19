# Windows CI build caches

`.github/actions/setup-build-tools/action.yml` restores two installed tools:

- **CMake:** the pinned distribution in the runner tool cache, including its
  completion marker. The setup action still activates its `bin` directory, but
  a cache hit avoids downloading and extracting it again.
- **MSYS2:** a dedicated installation under the runner tool cache, not the
  preinstalled `C:\msys64`. An exact cache hit skips MSYS2 setup, package updates
  and installation. The key includes the runner image, architecture, setup
  action contents and an explicit toolchain revision. Downloaded package
  archives are removed before saving this installed environment.

## Refresh and verification

To update MSYS2 packages without changing the image or package list, bump the
`toolchain-revision` default in the setup action. To update CMake, change its
`cmake-version` default. MSYS2 packages remain frozen between cache refreshes.

After pushing, let the first successful run populate the caches, then rerun
the same workflow. On the warm run, both distribution caches should report
hits and **Install MSYS2 on a cache miss** should be skipped. Compare total
setup time, including archive extraction, rather than download time alone.

Merge and run on `master` to seed caches readable by new pull requests. A
cache saved only by a PR remains scoped to that PR. Configuration probes and
compiler results retain their own independent caches.
