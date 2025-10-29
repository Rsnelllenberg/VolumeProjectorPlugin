# Introduction video
https://github.com/user-attachments/assets/a50c668d-30e0-4cd2-a6c8-6574041d27f9

# Basic instruction video
It explains how to use the program and its features from start (uploading the data) to finish, as well as points out some of its current quirks: 
TODO add video

# Build instructions
- Use the main branch. The other two branches are deprecated and contain a lot of unnecessary code that was used for testing the performance of the method.
- This is a plugin for the manivault studio framework: https://github.com/ManiVaultStudio . Go to the repository of the framework and it has extensive explination about how to built and run plugins for it. Though currently, instead of the core repository, this plugin still requires [a fork](https://github.com/Rsnelllenberg/core_volume) I made, which adds a custom dataset type.
- It has a dependency on the hnsw ANN library, and an optional dependency for the Faiss ANN library
- Before running the program, you might need to adjust some parameters in the DRVViewPlugin/volumeRenderer.h file:
  - _fullGPUMemorySize = Tells the program how much VRAM it can use (mainly important for the full data pipeline mode, apart from some warnings). The default is 2 GB
  - _hnswIndexFolder = Folder path where it saves/loads the constructed index to/from when using the hnsw ANN library, default: "C:/hnsw_index/"  (TODO make this a CMAKE parameter)
  - _hnswM, _hnswEfConstruction, _hwnsEfSearch = Parameters for the ANN algorithm library that is used for the full data pipeline rendermode
  - (only if you use FAISS) _nlist, _nprobe, _useFaissANN = Again, parameters for the FAISS ANN algorithms used in the full data pipeline rendermode
- You still need to add at least a dimensionality reduction plugin for this plugin to be usable
- Some example enviroments (datasets) can be found here: https://www.dropbox.com/scl/fo/0f39iny482c9hq630l0vh/AIML2mluR1Z37efwVjW2WMU?rlkey=pjctyfgva26y4ot0lk3qa1d1q&st=fg8m0d21&dl=0

# Misc

- This repository was originally built for a master's thesis project, and is as of yet not fully polished yet:
  - It has branches for benchmarking the code, which are quite messy and circumstantial(as they replace some of the functionality to make the testing easier), and should not be used 
  - The VolumeData dataset type needed to be shared between all plugins, and I piggybacked on the code in the core repository by making a fork. This fork is still required to run the current plugin as the datatype has not been transferred yet.
  - The code is also not completely bug-free, though it is generally in a usable state.
  - Finally, there are still a lot of possible QOL changes that I did not come around to add
- The thesis for which this plugin was originally built: https://resolver.tudelft.nl/uuid:91d45452-416f-4fda-bfb5-261be169f958

# Code overview
The repository actually contains 3 separate plugins:
- DRVTransferFunction: is responsible for the transfer function widget, which is mostly responsible for creating the textures that are used by the DRVViewPlugin to describe the transfer function 
- DRVViewPlugin: Does the actual volumetric rendering and contains all the necessary shaders as well as any extra processing that possibly needs to happen
  - The most important files here are the volumeRenderer files, which actually handle most of the rendering logic.
- DRVVolumeLoader: This is a dataloader plugin, and its only job is to either take a binary file (with some extra information provided by the user) or a point cloud that has a separate spatial and value dataset (as is sometimes obtained from unpacking hf5d files). And convert them to the VolumeData dataset type that the plugins work with

All plugins have their UI elements in the Action folder, contained in their specific plugin folder.
And generally, the file ending in ...Plugin.cpp / ... Plugin.h handles the data linking and setup of the plugin, while the file ending in Widget handles most of the underlying data (unless it offloads some of its logic to other auxiliary classes specific to the plugin)
The res (resource) folder contains any textures and/or shaders
