This is a light fork of Dolphin, intended to make rebasing PrimeHack on recent commits easier.
The result is packaged at `aur/dolphin-emu-primehack-git`.

This repo is organized into branches as follows:

* `main` – Contains this document and license information.
* `dolphin` – Synced with upstream Dolphin.
    * Some commits are tagged `#.#.#` for reference.
    * They do not correspond with upstream tags.
    * They do not follow semver conventions.
* `primehack-#.#.#` – Contain the PrimeHack patches rebased to tagged commits.
* `primehack-aur` – Synced with the latest `primehack-#.#.#` branch.
    * This is the branch the AUR package builds from.
* `wip` – Testing and staging.  Not for general use.  Frequently rebased and force pushed.  
