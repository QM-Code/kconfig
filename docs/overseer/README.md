# Rewrite Overseer Mode

This folder is for multi-repo integration workflow (`m-rewrite` + `m-dev` + `KARMA-REPO`).

Use this setup when you need integration-level oversight across all three repos:

1. Create a workspace directory:
   - `mkdir -p ~/dev/bz3-rewrite`
   - `cd ~/dev/bz3-rewrite`
2. Clone `KARMA-REPO`:
   - `git clone --branch main https://github.com/QM-Code/karma.git KARMA-REPO`
3. Clone `m-dev`:
   - `git clone --branch m-dev https://github.com/QM-Code/bz3.git m-dev`
4. Clone `m-rewrite`:
   - `git clone --branch m-rewrite https://github.com/QM-Code/bz3.git m-rewrite`
5. Initialize workspace bootstrap:
   - `cp m-rewrite/docs/overseer/README.init README.md`
6. Start Codex from `~/dev/bz3-rewrite/` and follow that README.

For normal standalone `m-rewrite` development, use repository root `README.md` instead.
