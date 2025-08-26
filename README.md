# Introduction video
  - ADD to git

# Basic instruction video
 - ADD to git

#Build instructions
- This is a plugin for the manivault studio framework: https://github.com/ManiVaultStudio . Go to the repository of the framework and it has extensive explination about how to built and run plugins for it. Though currently instead of the core repository, this plugins still requires a fork I made https://github.com/ManiVaultStudio , that adds a custom dataset type.
- It has a dependancy on the hnsw ANN library, and a optional dependancy for the Faiss ANN library

# Misc
- You still need to add at least a dimensionality reduction plugin for this plugin to be useable
- This repository was originally built for a thesis project, and is as of yet not fully polished yet:
  - It has branches for benchmarking the code and it still uses absolute paths of my PC
  - The VolumeData dataset type needed to be shared between all plugins and I piggybacked on the code in the core repository by making a fork, this fork is still required to run the current plugin as the datatype has not been transfered yet: C
  - The code also is not completely bug free, though it is generally in a useable state.
  - Finally there are still a lot of possible QOL changes that I did not come around to add
- The thesis for which this plugin was originally built: -
