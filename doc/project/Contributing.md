# Contributing

## What to contribute

We obviously welcome all bug fixes and general improvements! We
welcome new features to the extent that they are of general interest
to the Contiki-NG community. When it comes to communication protocols,
we focus on standardized solutions. New platforms are welcome, if they
fullfil the rules in [doc:new-platforms].

For features, protocols, or platforms that do not have their place in the
main repository, we will be happy to add to the wiki a link to the
contributor's repository.

## How to contribute

Contributions to Contiki-NG must be sent via a GitHub Pull Request (see [github:about-pull-requests]).
In short, you first need to clone the repository, push the changes as a branch of your copy, and submit a pull request from there.

Please adhere to the following guidelines:
* Make sure your pull request has one single focus.
* Identify the right target branch (see next section).
* Rebase on the latest version of the target branch and clean up the code (e.g. no dead or commented-out code) and the commit history. "git rebase -i" is useful for this purpose.
* For new features, GitHub Action tests are mandatory. For bug fixes, they are strongly encouraged.
* For new protocols, platforms, or major modules, we will ask for documentation, as a wiki page.
* Doxygen comments are strongly encouraged.
* Ensure that all contributed files have a valid copyright statement, open-source license, and follow the required code style and naming conventions (see [doc:code-style]). Adhere to ISO C99 in all C language source files.
* Write a descriptive pull request message. Explain the advantages and disadvantages of your proposed changes.
* If your PR introduces changes that need reflected in some pages in the wiki, make sure to point out which wiki pages are affected as part of your PR message. Providing updated text for those pages is always welcome.
 
## To which branch do I submit my PR?

In compliance with our Gitflow process [doc:development-cycle]:
* If it is a new feature or a major change, always submit to `develop`.
* If it is a bug fix or minor enhancement, `release-X.Y` if any, else `develop`.
* If it is a hot fix (removes a critical fault that would crash the device or leak data), `release-X.Y` if any, else `master`.

## Pull request merging policy

Pull requests (PRs) are reviewed by the [merge team](https://github.com/orgs/contiki-ng/teams/maintainers/members).
It is up to the maintainers to decide if their review alone is sufficient for merging.
This is generally the case for minor changes, removal of unused code, typo fixing etc.
This may also apply to code that requires specific expertise such as certain platforms or modules.
Whenever the reviewer feels that at least another pair of eyes is needed, they simply add the label "review needed".
In most cases, one or two approvals is regarded as enough for merging.

Passing all continuous integration tests is a strict requirement before merging.

## GitHub Actions continuous integration

All pull requests to Contiki-NG are automatically executed through our GitHub Actions CI workflow.

A box with information about the state of you pull request should show up at the bottom of the PR.

Note that you can also enable GitHub Actions on your Contiki-NG clone -- testing should work out-of-the-box.
You then get an overview of the state of each of your branches.

If the test fails it is likely that something is wrong with your code.
Please look carefully at the log and try to fix the problem in your branch.
If you have good reasons to believe the problem is independent from your contribution, mention this in your pull request and open an issue.

## New platforms

For new platforms we have the following requirements:
* There must be at least one person willing and committed to maintain it.
* The hardware must be commercially available or publicly usable (e.g. in a testbed).
* The port must demonstrate a certain degree of completeness and maturity. Common sense applies here.
* The port must provide compile regression tests by extending the existing CI testing framework.
* The port must be accompanied with documentation as a wiki page (e.g. [doc:platform-zoul]).

[github:about-pull-requests]: https://help.github.com/articles/about-pull-requests/
[doc:code-style]: /doc/project/Code-style
[doc:new-platforms]: #new-platforms
[doc:platform-zoul]: /doc/platforms/zolertia/zoul
[doc:development-cycle]: /doc/project/Development-cycle
