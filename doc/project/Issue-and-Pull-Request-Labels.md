# Issue and Pull Request Labels

Contiki-NG makes use of github labels for issues and pull requests.

Contributors cannot set labels, those are set by the maintainer team. This page briefly lists and explains the meaning of labels.

## Labels exclusive to PRs
Those will generally start with `pr/`.

* `pr/bugfix`: This pull request implements a bugfix. If the bugfix has been reported previously as an issue, this PR will normally also close the respective issue.
* `pr/enhancement`: This PR implements a new feature or a cleanup.
* `pr/review-needed`: The contributor has finished working on this PR, but is requesting feedback on it before it can be merged.
* `pr/work-in-progress`: Signifies that this PR has been opened so that people can help with it or contribute to a discussion about it, but it is not ready for pulling in.
* `documentation-required`: This pull is fundamentally desirable (whether it be a new feature or a bug fix), but doxygen API documentation must be added to it before it can be accepted.

## Labels exclusive to Issues

### Roadmap
Roadmap-related labels are used for issues that are used to document upcoming functionality. Normally, those issues will be assigned to a milestone. The `roadmap` label will normally be used for issues that correspond to features intended for the next release. The `roadmap/long-term` label is used for issues that are part of a long-term feature wishlist.

### Bugs
Issues that report a bug will have the `bug` label, plus a label that starts with `bug/`, followed by its severity:

* `bug/vulnerability`: A security vulnerability. 
* `bug/critical`: The bug affects critical functionality and does not have a workaround. Fixes for bugs in this category may be eligible for immediate merging with branch `master`. Example: Complete failure of a feature.
* `bug/medium`: A bug that affects key functionality. A workaround is available, but it requires code modifications. Examples: A defect in the implementation of a networking protocol. A platform-independent example is not working for a specific platform.
* `bug/low`: A bug that has a straightforward workaround.

### Questions and answers
Issues that fundamentally contain a question on how to use Contiki-NG will be labelled with the `question` label. Questions that receive no response/reaction within a week will be labelled with `question/timeout` and closed.

## Generic labels
Those labels can be associated with a PR as well as with an issue.

* `question`: This PR/issue contains a question.
* `discussion`: This PR/issue contains a discussion about a specific Contiki-NG feature. For example, it can be a discussion on design choices related to an upcoming feature.

* `invalid`: This PR or issue is invalid and will get closed.
* `duplicate`: This PR or issue is a duplicate and will get closed.

* `documentation`: This is a PR that extends or fixes documentation, or an issue that identifies a problem in the documentation. It can contain changes to the source tree, or it can recommend a change to the wiki.

* `help-wanted`: This PR or issue requires changes that the original contributor is asking for assistance with.