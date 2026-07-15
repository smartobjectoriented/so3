.. _release_process:

Release process
###############

SO3 follows a branch-per-release model: development happens on ``main``, and
every minor version gets a long-lived maintenance branch on which patch releases
are tagged. This keeps stable lines alive for backports while ``main`` moves on.

Overview
********

Three git objects work together, and GitHub surfaces them in different places:

.. list-table::
   :header-rows: 1
   :widths: 22 20 58

   * - Object
     - Example
     - Role
   * - ``main`` branch
     - ``main``
     - Continuous development (the next, unreleased version).
   * - ``release/vX.Y`` branch
     - ``release/v6.2``
     - Long-lived maintenance line for a minor version. Patch fixes land here
       and are tagged.
   * - Tag ``vX.Y.Z``
     - ``v6.2.1``
     - Immutable point marking a delivered version. Release candidates use the
       ``-rc`` suffix (``v6.2.1-rc``).
   * - GitHub Release
     - "SO3 v6.2.1"
     - The release page (notes + assets), attached to a tag. Exactly one is
       flagged *Latest*; ``-rc`` tags are published as *pre-release*.

Versioning
**********

Versions follow semantic versioning ``vMAJOR.MINOR.PATCH``:

* **MAJOR** — breaking changes to the boot chain, ABI or on-disk formats.
* **MINOR** — new features, backward compatible. Opens a new ``release/vX.Y``
  branch.
* **PATCH** — bug fixes only, cut *on* an existing ``release/vX.Y`` branch.

Release candidates append ``-rc`` (optionally ``-rcN`` for successive
candidates), e.g. ``v6.3.0-rc``.

Branch layout
*************

::

   main ──●──●──●──●──●──●───────►   development (next version)
           \
   release/v6.2  ●──●──●             maintenance line for 6.2.x
                 │  │  └─ v6.2.1     (tags live on the branch)
                 │  └──── v6.2.1-rc
                 └─────── v6.2.0

While a minor line has not diverged from ``main`` yet (no work started on the
next minor), its ``release/vX.Y`` branch and ``main`` may point at the same
commit — that is expected.

Which fixes go on a release branch?
***********************************

Being a bug fix is *not* what sends a change to a release branch. ``main`` is
the continuous development line and carries both features **and** fixes as they
land; anything committed there simply ships in the next version. There are two
kinds of fix:

* **A fix for unreleased code, or one that can wait for the next version.**
  Nothing special — it is an ordinary commit on ``main`` and ships in the next
  ``vX.Y.0``. Do *not* touch any release branch.
* **A fix for an already-published version** (e.g. a bug in ``v6.2.1``) that must
  ship *before* the next version. Only this case uses the patch-release
  procedure below: the fix lands on ``release/v6.2`` (tagged ``v6.2.2``) and is
  also carried to ``main`` so it is not lost at the next minor.

In other words, what sends a change to ``release/vX.Y`` is the need to patch a
*live, already-released* version — never the mere fact that it is a fix rather
than a feature.

Cutting a patch release (``vX.Y.Z``)
************************************

Patch fixes are committed **on the release branch**, then tagged. If a fix was
first merged into ``main``, cherry-pick it onto the branch rather than
fast-forwarding.

.. code-block:: sh

   git checkout release/v6.2
   git cherry-pick <sha>          # or commit the fix directly

   # optional: publish a candidate first
   git tag -a v6.2.1-rc -m "so3 v6.2.1-rc"
   git push origin release/v6.2 v6.2.1-rc
   gh release create v6.2.1-rc --title "SO3 v6.2.1-rc" \
       --target release/v6.2 --prerelease --generate-notes

   # final release
   git tag -a v6.2.1 -m "so3 v6.2.1"
   git push origin release/v6.2 v6.2.1
   gh release create v6.2.1 --title "SO3 v6.2.1" \
       --target release/v6.2 --latest --generate-notes

Cutting a new minor release (``vX.Y.0``)
****************************************

When ``main`` is ready for a new minor version, branch off it, then tag:

.. code-block:: sh

   git checkout main
   git checkout -b release/v6.3
   git push -u origin release/v6.3

   git tag -a v6.3.0 -m "so3 v6.3.0"
   git push origin v6.3.0
   gh release create v6.3.0 --title "SO3 v6.3.0" \
       --target release/v6.3 --latest --generate-notes

After tagging: propagate the release to ``main``
************************************************

A release is not finished when the tag is pushed. Two references to the
current version live on ``main`` and must be bumped right after every
release (they drift silently otherwise):

* the *Maintained versions* table in ``README.md`` (the *Latest release*
  column of the line, and the *Status* column when a new minor line starts);
* the build-system version pins: rename the ``so3_X.Y.Z.bb`` and
  ``avz_X.Y.Z.bb`` recipes (under ``build/meta-so3/recipes-so3/``) to the
  new version and update the ``PREFERRED_VERSION_so3`` /
  ``PREFERRED_VERSION_avz`` entries in ``build/conf/local.conf`` (see the
  ``v6.2.3`` bump for a template).

Rules of thumb
**************

* One ``release/vX.Y`` branch **per minor**, not per patch — patches are tags
  *on* the branch.
* Tags are immutable: never move or delete a published ``vX.Y.Z`` tag. To
  correct a release, cut the next patch.
* Exactly one GitHub Release carries the *Latest* flag; every ``-rc`` Release is
  a *pre-release* so it never shadows the latest stable version.
* Once a ``release/vX.Y`` branch has diverged from ``main``, backport fixes with
  ``git cherry-pick`` — do not fast-forward the branch onto ``main``.
