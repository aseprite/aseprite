# Code of Conduct

We have a [code of conduct](CODE_OF_CONDUCT.md) that we all must
read. Be polite to everyone. If you are not in your best day, take a
deep breath and try again. Smile ^_^

# New Issues

Before you submit an issue:

* Search in the current [list of issues](https://github.com/aseprite/aseprite/issues).
* If the issue already exists, add a +1, and you can click the `Subscribe` button to get notifications via email.

# Compilation problem

Before you submit an issue about a **compilation problem**, check
the following items:

* See how to get the source code correctly in the [INSTALL](INSTALL.md) guide.
* Check if you are using the latest repository clone.
* Remember that we use submodules, so you need to initialize and update them.
* Remember that might be some [pull requests](https://github.com/aseprite/aseprite/pulls)
  being reviewed to fix your same problem.
* If you are compiling on Mac OS X or on Linux, be patient, we don't
  compile on those platforms regularly.

# Contributing

One of the easiest ways to contribute is writting articles, blog posts,
recording video tutorials,
[creating pixel art](http://aseprite.deviantart.com/), or showing your love
to Aseprite e.g. naming Aseprite in your website and linking it to
http://www.aseprite.org/, following
[@aseprite](https://twitter.com/aseprite) twitter account, or
[giving a donation](http://www.aseprite.org/donate/).

Other ways to contribute require direct contact with us. For example:

* Writting documentation.
* Making art with Aseprite and for Aseprite (logos, skins, mockups).
* Sending patches for features or bug fixes.
* Reviewing issues in the [issue tracker](https://github.com/aseprite/aseprite/issues) and making comments.

## Documentation

You can start seeing the
[documentation](http://www.aseprite.org/docs/), and
[contact us](support@aseprite.org) if you want to help
writting documentation or recording [tutorials](http://www.aseprite.org/tutorial/).

If you are going to write documentation, we recommend you to take
screenshots or record a GIF animations to show steps:

* As screen recording software, on Windows you can generate GIF files
  using [LICEcap](http://www.cockos.com/licecap/).
* You can upload the PNG/GIF images to [Imgur](http://imgur.com/).

## Reviewing Issues

You can review issues, make comments, or create new ones (features,
bug reports, etc.) in the
[issue tracker](https://github.com/aseprite/aseprite/issues). You
are encouraged to create mockups for any issue you see and attach them.

## Hacking

The first thing to keep in mind if you want to modify the source code:
checkout the **master** branch. It is the branch that we use to
develop new features and fix issues that are planned for the next big
release. See the [INSTALL](INSTALL.md) guide to know how to compile.

To start looking the source code, see how it is organized in
[src/README.md](https://github.com/aseprite/aseprite/tree/master/src/#aseprite-source-code)
file.

## Forking & Pull Requests

You can fork the GitHub repository using the Fork button at
[https://github.com/aseprite/aseprite](https://github.com/aseprite/aseprite).

The Pull Requests (PR) systems works in this way:

1. First of all you will need to sign our
   [Contributor License Agreement](https://github.com/aseprite/opensource/blob/master/sign-cla.md#sign-the-cla) (CLA).
1. Then you can start working on Aseprite. Create a new branch from `master`, e.g. `fix-8` to fix the issue 8.
1. Start working on that new branch, and push your commits to your fork.
1. Create a new PR to merge your `fix-8` branch to the official `master`.
1. If the PR is accepted (does not require review/comments/modifications),
   your branch is merged into `master`.
1. You will need to pull changes from the official `master` branch, and
   merge them in your own `master` branch. Finally you can discard your
   own `fix-8` branch (because those changes should be already merged
   into `master` if the PR was accepted).
1. Continue working from the new `master` head.

To keep in mind: **always** start working from the `master` head, if you
want to fix three different issues, create three different branches
from `master` and then send three different PR. Do not chain all the
fixes in one single branch. E.g. `fix-issues-3-and-8-and-25`.

## Mailing List

Subscribe to
[aseprite-discuss](http://groups.google.com/group/aseprite-discuss)
group. If you want to start working in something
(e.g. [an open issue](https://github.com/aseprite/aseprite/issues)),
you should send an email to the group describing what you want to do,
to avoid programming in something that is already done or which
someone else is working on.

And always remember to take a look at our
[roadmap](http://www.aseprite.org/roadmap/).
