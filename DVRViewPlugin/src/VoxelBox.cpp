//#include "VoxelBox.h"
//#include <QDebug>
//
//
//void VoxelBox::init(int xVoxels, int yVoxels, int zVoxels){
//    _xVoxels = xVoxels;
//    _yVoxels = yVoxels;
//    _zVoxels = zVoxels;
//
//    _voxels.resize(_xVoxels * _yVoxels * _zVoxels);
//
//    _volumeTexture.create();
//    _volumeTexture.initialize();
//    qDebug() << "VoxelBox initialized";
//}
//
//void VoxelBox::updateBounds() {
//    if (_spatialData.empty()) return;
//
//    float minX = _spatialData[0], maxX = _spatialData[0];
//    float minY = _spatialData[1], maxY = _spatialData[1];
//    float minZ = _spatialData[2], maxZ = _spatialData[2];
//
//    for (size_t i = 0; i < _spatialData.size(); i += 3) {
//        float x = _spatialData[i];
//        float y = _spatialData[i + 1];
//        float z = _spatialData[i + 2];
//
//        if (x < minX) minX = x;
//        if (x > maxX) maxX = x;
//        if (y < minY) minY = y;
//        if (y > maxY) maxY = y;
//        if (z < minZ) minZ = z;
//        if (z > maxZ) maxZ = z;
//    }
//
//    _bounds.setBounds(minX, maxX, minY, maxY, minZ, maxZ);
//}
//
//// Finds the optimal dimensions for the volume cube when given a certain amount of cubes
//// Finds the optimal dimensions for the volume cube when given a certain amount of cubes
//mv::Vector3f VoxelBox::findOptimalDimensions(int N, mv::Vector3f maxDims) {
//    int maxX = std::min(int(std::ceil(std::cbrt(N))), int(std::floor(maxDims.x)));
//    int maxY = std::min(int(std::ceil(std::cbrt(N))), int(std::floor(maxDims.y)));
//    int maxZ = std::min(int(std::ceil(std::cbrt(N))), int(std::floor(maxDims.z)));
//
//    mv::Vector3f bestDims(1, 1, 1);
//    int bestVolume = std::numeric_limits<int>::max();
//
//    for (int z = 1; z <= maxZ; ++z) {
//        if (N % z != 0) continue;
//        int remaining = N / z;
//        for (int y = 1; y <= maxY; ++y) {
//            if (remaining % y != 0) continue;
//            int x = remaining / y;
//            if (x <= maxX) {
//                int volume = x * y * z;
//                if (volume == N) {
//                    return mv::Vector3f(x, y, z); // Exact match found
//                }
//                if (volume >= N && volume < bestVolume) {
//                    bestVolume = volume;
//                    bestDims = mv::Vector3f(x, y, z); // best match so far
//                }
//            }
//        }
//    }
//
//    return bestDims; // Return the closest combination found
//}
//
//
//
//void VoxelBox::setData(const std::vector<float>& spatialData, const std::vector<float>& valueData, int numValueDimensions) {
//    _spatialData = spatialData;
//    _valueData = valueData;
//    _numValueDimensions = numValueDimensions;
//
//    updateBounds();
//    updateTexture(numValueDimensions);
//}
//
//void VoxelBox::updateTexture(int numValueDimensions)
//{
//    //voxelize();
//     
//    // Retrieve dimensions from _voxelBox
//    // Add 2 to each dimension to create a border voxels to avoid weird interpolation issues
//    int width = int(_xVoxels); 
//    int height = int(_yVoxels);
//    int depth = int(_zVoxels);
//
//
//    int brickAmount = std::ceil(numValueDimensions / 4);
//    mv::Vector3f maxDimsInBricks = mv::Vector3f(GL_MAX_3D_TEXTURE_SIZE / _xVoxels, GL_MAX_3D_TEXTURE_SIZE / _yVoxels, GL_MAX_3D_TEXTURE_SIZE / _zVoxels);
//    _brickLayout = findOptimalDimensions(brickAmount, maxDimsInBricks);
//
//    qDebug() << "Brick dims: " << _brickLayout.x << _brickLayout.y << _brickLayout.z;
//    int trueWidth = width * int(_brickLayout.x);
//    int trueHeight = height * int(_brickLayout.y);
//    int trueDepth = depth * int(_brickLayout.z);
//
//    std::vector<float> textureData(width * height * depth * numValueDimensions, 0.0f);
//    int numPoints = _spatialData.size() / 3;
//    for (int i = 0; i < numPoints; i += 3) {
//        mv::Vector3f normalizedPos = normalizePosition(mv::Vector3f(_spatialData[i], _spatialData[i + 1], _spatialData[i + 2]));
//        int x = int(std::round(normalizedPos.x));
//        int y = int(std::round(normalizedPos.y));
//        int z = int(std::round(normalizedPos.z));
//        
//        //Populate the texture data with values 
//        for (int j = 0; j < numValueDimensions; ++j) {
//            int brickIndex = std::floor(j / 4);
//
//            int brickX = brickIndex % int(_brickLayout.x);
//            int brickY = int(std::floor(brickIndex / _brickLayout.x)) % int(_brickLayout.y);
//            int brickZ = std::floor(brickIndex / (_brickLayout.x * _brickLayout.y));
//
//            int voxelIndex =  (x + (brickX * width)) * 4 
//                            + (y + (brickY * height)) * trueWidth * 4
//                            + (z + (brickZ * depth)) * trueWidth * trueHeight * 4;
//            textureData[voxelIndex + (j % 4)] = _valueData[i * numValueDimensions + j];
//        }
//
//    }
//
//    // Generate and bind a 3D texture
//    _volumeTexture.bind();
//    _volumeTexture.setData(trueWidth, trueHeight, trueDepth, textureData);
//    _volumeTexture.release(); // Unbind the texture
//
//    containsData = true;
//}
//
////void VoxelBox::voxelize() {
////    int numPoints = _spatialData.size() / 3;
////    for (int i = 0; i < numPoints; i += 3) {
////        qDebug() << "Voxelizing before" << _spatialData[i] << _spatialData[i + 1] << _spatialData[i + 2];
////        mv::Vector3f normalizedPos = normalizePosition(mv::Vector3f(_spatialData[i], _spatialData[i + 1], _spatialData[i + 2]));
////        int x = int(std::round(normalizedPos.x));
////        int y = int(std::round(normalizedPos.y));
////        int z = int(std::round(normalizedPos.z));
////        qDebug() << "Voxelizing after" << x << y << z;
////        int voxelIndex = getVoxelIndex(x, y, z);
////
////        Voxel& voxel = _voxels[voxelIndex];
////        voxel.position = mv::Vector3f(x, y, z);
////
////        if (voxel.values.empty()) {
////            voxel.values.assign(_valueData.begin() + i * _numValueDimensions, _valueData.begin() + (i + 1) * _numValueDimensions);
////        }
////        else {
////            // If voxel already exists, average the values
////            for (int j = 0; j < _numValueDimensions; ++j) {
////                voxel.values[j] = (voxel.values[j] + _valueData[i * _numValueDimensions + j]) / 2.0f;
////            }
////        }
////    }
////}
//
//
//mv::Vector3f VoxelBox::getBrickLayout() {
//    return _brickLayout;
//}
//
//size_t VoxelBox::getBoxSize() const {
//    return _xVoxels * _yVoxels * _zVoxels;
//}
//
//const Voxel& VoxelBox::getVoxel(int x, int y, int z) const {
//    size_t voxelIndex = getVoxelIndex(x, y, z);
//    return _voxels[voxelIndex];
//}
//
//mv::Texture3D& VoxelBox::getVolumeTexture() {
//    return _volumeTexture;
//}
//
//mv::Vector3f VoxelBox::getDims() const {
//    return { static_cast<float>(_xVoxels), static_cast<float>(_yVoxels), static_cast<float>(_zVoxels) };
//}
//
//mv::Vector3f VoxelBox::getCenter() const {
//    return { _xVoxels / 2.0f, _yVoxels / 2.0f, _zVoxels / 2.0f };
//}
//
//void VoxelBox::setBoxSize(int xVoxels, int yVoxels, int zVoxels) {
//    _xVoxels = xVoxels;
//    _yVoxels = yVoxels;
//    _zVoxels = zVoxels;
//    _voxels.resize(_xVoxels * _yVoxels * _zVoxels);
//    updateTexture(_numValueDimensions);
//}
//
//mv::Vector3f VoxelBox::normalizePosition(const mv::Vector3f& pos) const {
//    mv::Vector3f normalizedPos;
//    normalizedPos.x = (pos.x - _bounds.getLeft()) / (_bounds.getWidth()) * (_xVoxels - 1);
//    normalizedPos.y = (pos.y - _bounds.getBottom()) / (_bounds.getHeight()) * (_yVoxels - 1);
//    normalizedPos.z = (pos.z - _bounds.getFront()) / (_bounds.getDepth()) * (_zVoxels - 1);
//    return normalizedPos;
//}
//
//size_t VoxelBox::getVoxelIndex(int x, int y, int z) const {
//    return x + _xVoxels * (y + _yVoxels * z);
//}
