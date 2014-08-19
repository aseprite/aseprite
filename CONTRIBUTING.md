# Contributing

You can contribute to Aseprite in several ways. One of the easiest
ways is writting articles, blog posts, recording video tutorials,
[creating pixel art](http://aseprite.deviantart.com/), or showing your love
to Aseprite e.g. naming Aseprite in your website and linking it to
http://www.aseprite.org/, following
[@aseprite](https://twitter.com/aseprite) twitter account, or
[giving a donation](http://www.aseprite.org/donate/).

Other ways to contribute require direct contact with us. For example:
* Writting documentation.
* Making art *for* Aseprite (logos, skins, mockups).
* Sending patches for features or bug fixes.
* Reviewing issues in the [issue tracker](https://github.com/aseprite/aseprite/issues) and making comments.

The following sections explain some tips about each of these points.

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

## Issues

You can review issues, make comments, or create new ones (features,
bug reports, etc.) in our
[issue tracker](https://github.com/aseprite/aseprite/issues).  You
are encouraged to create mockups for any issue you see and attach them.

## Hacking

The first thing to keep in main if you want to modify the source code:
checkout the **master** branch. It is the branch that we use to
develop new features and fix issues that are planned for the next big
release.

To start looking the source code, see how it is organized in
[src/README.md](https://github.com/aseprite/aseprite/tree/master/src/#aseprite-source-code)
file.

## Get the Source Code

If you want to help in Aseprite, first of all you will need a fresh
copy of the Git repository. It is located in GitHub, right here:

https://github.com/aseprite/aseprite

You can clone it locally using the following command (read-only URL):

    git clone git://github.com/aseprite/aseprite.git

On Windows you can use programs like
[msysgit](http://msysgit.github.io/) to clone the repository.

## Compiling

See [INSTALL.md](INSTALL.md) to know how to compile Aseprite.

## Forking & Pull Requests

You can fork the GitHub repository using the Fork button at
[https://github.com/aseprite/aseprite](https://github.com/aseprite/aseprite).

The Pull Requests (PR) systems works in this way:

1. You've to create a new branch from `master`, e.g. `fix-8` to fix the issue 8.
1. Start working on that new branch, and push that branch to your fork.
1. Create a new PR to merge your `fix-8` branch to official `master`.
1. If the PR is accepted, your branch is merged into `master`.
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

And always remember to look the
[Roadmap](http://www.aseprite.org/roadmap/).
