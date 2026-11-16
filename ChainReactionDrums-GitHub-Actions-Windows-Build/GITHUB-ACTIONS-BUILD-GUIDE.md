# Build a Windows VST3 automatically with GitHub Actions

This archive contains the Chain Reaction Drums JUCE source plus a GitHub Actions workflow.
GitHub's Windows runner compiles it; you do not need Visual Studio or CMake installed locally.

## One-time setup

1. Create or sign in to a GitHub account at https://github.com.
2. Click **New repository**. Name it `ChainReactionDrums`. Choose **Private** or **Public**, then click **Create repository**.
3. Extract this ZIP on your PC.
4. Open the extracted `ChainReactionDrums` folder. It should contain `CMakeLists.txt`, `Source`, `README.md`, and `.github`.
5. On your new GitHub repository page, choose **Add file → Upload files**.
6. Drag the *contents* of the extracted `ChainReactionDrums` folder into the upload area. Make sure `.github/workflows/build-windows-vst3.yml` is included. Commit the upload to `main`.

## Start the build

1. In the repository, click the **Actions** tab.
2. If GitHub asks to enable workflows, click **I understand my workflows, go ahead and enable them**.
3. Select **Build Windows VST3** in the left sidebar.
4. Click **Run workflow**, keep branch `main`, then click the green **Run workflow** button.
5. Wait for the run to finish. A green check means the build job succeeded. If it fails, open the run and click the failed step to see the error log.

## Download and install

1. Open the successful workflow run.
2. Scroll to the bottom to **Artifacts**.
3. Download `ChainReactionDrums-Windows-x64-VST3`.
4. Extract that downloaded ZIP. It should contain `Chain Reaction Drums.vst3` (a folder/bundle).
5. Copy the entire `.vst3` folder to:
   `C:\Program Files\Common Files\VST3`
   (Create the `VST3` folder if it does not exist. You may need administrator permission.)
6. Restart FL Studio, open **Options → Manage plugins**, and click **Find installed plugins**. Search for **Chain Reaction Drums**.

## If the build fails

Open the failed Actions run, expand the first red step, and share a screenshot or copy the error text here. Do not download an artifact from a failed run.

## Important feature status

This workflow only automates compiling the source that is in this archive. It does **not** add the unfinished features listed in the source README. In particular, five-minute rolling recovery, real take recording/stem export, per-node chance sliders, advanced roll/burst/stutter generators, true tape-stop playback, and arranged half-time sections are still unfinished. A successful compile confirms the binary was built, not that every desired feature exists or has been validated inside FL Studio.
