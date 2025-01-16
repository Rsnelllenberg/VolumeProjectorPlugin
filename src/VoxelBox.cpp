#include "VoxelBox.h"
#include <QDebug>


void VoxelBox::init(int xVoxels, int yVoxels, int zVoxels){
    _xVoxels = xVoxels;
    _yVoxels = yVoxels;
    _zVoxels = zVoxels;

    _voxels.resize(_xVoxels * _yVoxels * _zVoxels);

    _volumeTexture.create();
    _volumeTexture.initialize();
    qDebug() << "VoxelBox initialized";
}

void VoxelBox::updateBounds() {
    if (_spatialData.empty()) return;

    float minX = _spatialData[0], maxX = _spatialData[0];
    float minY = _spatialData[1], maxY = _spatialData[1];
    float minZ = _spatialData[2], maxZ = _spatialData[2];

    for (size_t i = 0; i < _spatialData.size(); i += 3) {
        float x = _spatialData[i];
        float y = _spatialData[i + 1];
        float z = _spatialData[i + 2];

        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
        if (z < minZ) minZ = z;
        if (z > maxZ) maxZ = z;
    }

    _bounds.setBounds(minX, maxX, minY, maxY, minZ, maxZ);
}

void VoxelBox::setData(const std::vector<float>& spatialData, const std::vector<float>& valueData, int numValueDimensions) {
    _spatialData = spatialData;
    _valueData = valueData;
    _numValueDimensions = numValueDimensions;

    updateBounds();
    voxelize();

    // Retrieve dimensions from _voxelBox
    int width = static_cast<int>(_xVoxels);
    int height = static_cast<int>(_yVoxels);
    int depth = static_cast<int>(_zVoxels);

    

    // Fill the texture with data from _voxelBox
    std::vector<float> textureData(width * height * depth, 0.0f);

    for (const Voxel& voxel : _voxels) {
        int x = static_cast<int>(voxel.position.x);
        int y = static_cast<int>(voxel.position.y);
        int z = static_cast<int>(voxel.position.z);
        size_t index = x + y * width + z * width * height;
        if (index < textureData.size()) {
            textureData[index] = voxel.values.empty() ? 0.0f : voxel.values[0]; // Assuming the first value is used
        }
    }

    // Generate and bind a 3D texture
    _volumeTexture.bind();
    _volumeTexture.setData(width, height, depth, textureData);
    _volumeTexture.release(); // Unbind the texture

    containsData = true;
}

void VoxelBox::voxelize() {
    int numPoints = _spatialData.size() / 3;
    for (size_t i = 0; i < numPoints; i++) {
        mv::Vector3f normalizedPos = normalizePosition(mv::Vector3f(_spatialData[3*i], _spatialData[3*i+1], _spatialData[3*i+ 2]));
        int x = static_cast<int>(std::round(normalizedPos.x));
        int y = static_cast<int>(std::round(normalizedPos.y));
        int z = static_cast<int>(std::round(normalizedPos.z));
        size_t voxelIndex = getVoxelIndex(x, y, z);

        Voxel& voxel = _voxels[voxelIndex];
        voxel.position = mv::Vector3f(x, y, z);

        if (voxel.values.empty()) {
            voxel.values.assign(_valueData.begin() + i * _numValueDimensions, _valueData.begin() + (i + 1) * _numValueDimensions);
        }
        else {
            // If voxel already exists, average the values
            for (size_t j = 0; j < _numValueDimensions; ++j) {
                voxel.values[j] = (voxel.values[j] + _valueData[i * _numValueDimensions + j]) / 2.0f;
            }
        }
    }
}

size_t VoxelBox::getBoxSize() const {
    return _xVoxels * _yVoxels * _zVoxels;
}

const Voxel& VoxelBox::getVoxel(int x, int y, int z) const {
    size_t voxelIndex = getVoxelIndex(x, y, z);
    return _voxels[voxelIndex];
}

mv::Texture3D& VoxelBox::getVolumeTexture() {
    return _volumeTexture;
}

mv::Vector3f VoxelBox::getDims() const {
    return { static_cast<float>(_xVoxels), static_cast<float>(_yVoxels), static_cast<float>(_zVoxels) };
}

mv::Vector3f VoxelBox::getCenter() const {
    return { _xVoxels / 2.0f, _yVoxels / 2.0f, _zVoxels / 2.0f };
}

void VoxelBox::setBoxSize(int xVoxels, int yVoxels, int zVoxels) {
    _xVoxels = xVoxels;
    _yVoxels = yVoxels;
    _zVoxels = zVoxels;
    _voxels.resize(_xVoxels * _yVoxels * _zVoxels);
    voxelize();
}

mv::Vector3f VoxelBox::normalizePosition(const mv::Vector3f& pos) const {
    mv::Vector3f normalizedPos;
    normalizedPos.x = (pos.x - _bounds.getLeft()) / (_bounds.getWidth()) * (_xVoxels - 1);
    normalizedPos.y = (pos.y - _bounds.getBottom()) / (_bounds.getHeight()) * (_yVoxels - 1);
    normalizedPos.z = (pos.z - _bounds.getFront()) / (_bounds.getDepth()) * (_zVoxels - 1);
    return normalizedPos;
}

size_t VoxelBox::getVoxelIndex(int x, int y, int z) const {
    return x + _xVoxels * (y + _yVoxels * z);
}
