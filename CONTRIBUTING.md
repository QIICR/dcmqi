Contributing to dcmqi
===========================

There are many ways to contribute to dcmqi, with varying levels of effort.  Do try to
look through the documentation first if something is unclear, and let us know how we can
do better.

  * Ask a question on the [dcmqi google group](https://groups.google.com/forum/#!forum/dcmqi)
  * Submit a feature request or bug, or add to the discussion on the [dcmqi issue tracker](https://github.com/QIICR/dcmqi/issues)
  * Submit a [Pull Request (PR)](https://github.com/QIICR/dcmqi/pulls) to improve dcmqi or its documentation

We encourage a range of Pull Requests, from patches that include passing tests and
documentation, all the way down to half-baked ideas that launch discussions.

The PR Process, Continuous Integration, and Related Gotchas
----------------------------------------------

#### How to submit a PR ?

If you are new to dcmqi development and you don't have push access to the dcmqi
repository, here are the steps:

1. [Fork and clone](https://help.github.com/articles/fork-a-repo/) the repository.
3. Create a branch.
4. [Push](https://help.github.com/articles/pushing-to-a-remote/) the branch to your GitHub fork.
5. Create a [Pull Request](https://github.com/QIICR/dcmqi/pulls).

This corresponds to the `Fork & Pull Model` mentioned in the [GitHub flow](https://guides.github.com/introduction/flow/index.html)
guides.

If you have push access to dcmqi repository, you could simply push your branch
into the main repository and create a [Pull Request](https://github.com/QIICR/dcmqi/pulls). This corresponds to the
`Shared Repository Model` and will facilitate other developers to checkout your
topic without having to [configure a remote](https://help.github.com/articles/configuring-a-remote-for-a-fork/).
It will also simplify the workflow when you are _co-developing_ a branch.

Based on the comments posted by the reviewers of your PR, you may have to revisit your patches.

#### How to integrate a PR ?

Getting your contributions integrated is relatively straightforward, here
is the checklist:

* All tests pass
* Consensus is reached. This usually means that at least one reviewer approved your contribution
and a reasonable amount of time passed without anyone objecting.

Next, there are two scenarios:
* You do NOT have push access: A dcmqi core developer will integrate your PR.
* You have push access: Simply click on the "Merge pull request" button.

Then, click on the "Delete branch" button that appears afterward.

#### Automatic testing of pull requests

Every pull request is tested automatically using continuous integration using CircleCI, Appveyor and TravisCI
each time you push a commit to it. No PR should be merged until all CI are green, unless there is
a good reason to merge it first (as an example, proper testing cannot be done due to the references of
  the components that are changing, but must be available in the `master` branch).

#### Documentation updates

If you contribute a change that will add a new module/function to dcmqi, you are encouraged
to add documentation of the new feature. We use [Gitbook](https://www.gitbook.com/book/fedorov/dcmqi/details) for
maintaining dcmqi documentation.
