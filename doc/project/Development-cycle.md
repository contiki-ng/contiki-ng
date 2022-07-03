# Development cycle

The Contiki-NG development cycle follows [Gitflow][gitflow].

The `master` branch contains official releases only.
Development takes place in the `develop` branch, with all contributions done through pull requests (user-hosted feature branch).
Features are merged when they are deemed useful and fulfill the contribution guidelines (see [doc:contributing]).
They get consolidated gradually, as more and more developers use them.
Continuous integration guarantees non-regression of `develop`.

Twice a year -- once in the fall and in the spring, a new release is planned.
A tag is added on `develop`, e.g. for release X.Y, `develop/vX.Y`, and a new branch `release-X.Y` is created.
While normal development continues on `develop`, the release branch undergoes a feature-freeze:
only bug fixes and pre-release polishing are done on `release-X.Y`. In the long term, all tags prefixed with `develop/vX.Y` will indicate the point in git history where the `release-X.Y` branch was created.

Meanwhile, the maintainers carry out more extensive testing of various features on different hardware, check the consistency of documentation and tutorials, etc.
After a 2--4 weeks, when things stabilize, the release branch is merged into `master` as well as back into `develop` (so as to include all pre-release work in `develop`).
The release is tagged on `master`, e.g. tag `release/vX.Y`, the release branch deleted, and the release is announced. All release tags will be prefixed by `release/`

In case a *hot fix* (i.e. one that avoids a critical fault) is needed between releases, it will be submitted directly to `master` (either from a user-hosted branch or a dedicated `hotfix-xyz` branch in the main repo).
When the hot fix is ready, it is merged into `master` (in turned merged back to `develop`), and a new minor version is released.
Changelogs are kept at [doc:releases].

## Branches and tags at a glance

In summary, here is the meaning/naming convention used for the various tags and branches:
* **Branch** `master`: Contains releases only.
* *Tag* `release/vX.Y`: Marks the release of version X.Y. This will tag the respective release commit within `master`. The tag message will be `Release vX.Y`
* **Branch** `develop`: Main development branch.
* **Branch** `release-X.Y`: A transient branch hosting bug fixes and final polishing as part of the release of version vX.Y.
* *Tag* `develop/vX.Y`: Marks the point in branch `develop`'s git history where the `release-X.Y` branch was created. The tag message will be `Start of release branch for vX.Y`
* *Tag* `old/vX.Y`: Tags a release version in the git history of the vanilla Contiki OS.

[doc:contributing]: /doc/project/Contributing
[doc:releases]: https://github.com/contiki-ng/contiki-ng/releases
[gitflow]: https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow
