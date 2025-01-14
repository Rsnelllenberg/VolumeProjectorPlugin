#include "VoxelBox.h"

VoxelBox::VoxelBox(int xVoxels, int yVoxels, int zVoxels, const Bounds3D& bounds)
    : _xVoxels(xVoxels), _yVoxels(yVoxels), _zVoxels(zVoxels), _bounds(bounds) {
    _voxels.resize(_xVoxels * _yVoxels * _zVoxels);
}

void VoxelBox::setData(const std::vector<mv::Vector3f>& spatialData, const std::vector<std::vector<float>>& valueData) {
    _spatialData = spatialData;
    _valueData = valueData;
    voxelize();
}

void VoxelBox::voxelize() {
    for (size_t i = 0; i < _spatialData.size(); ++i) {
        mv::Vector3f normalizedPos = normalizePosition(_spatialData[i]);
        int x = static_cast<int>(std::round(normalizedPos.x));
        int y = static_cast<int>(std::round(normalizedPos.y));
        int z = static_cast<int>(std::round(normalizedPos.z));
        size_t voxelIndex = getVoxelIndex(x, y, z);

        Voxel& voxel = _voxels[voxelIndex];
        voxel.position = mv::Vector3f(x, y, z);

        if (voxel.values.empty()) {
            voxel.values = _valueData[i];
        }
        else {
            // If voxel already exists, average the values
            for (size_t j = 0; j < _valueData[i].size(); ++j) {
                voxel.values[j] = (voxel.values[j] + _valueData[i][j]) / 2.0f;
            }
        }
    }

    // Fill in empty voxels with zero values
    for (int x = 0; x < _xVoxels; ++x) {
        for (int y = 0; y < _yVoxels; ++y) {
            for (int z = 0; z < _zVoxels; ++z) {
                size_t voxelIndex = getVoxelIndex(x, y, z);
                Voxel& voxel = _voxels[voxelIndex];
                if (voxel.values.empty()) {
                    voxel.position = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(z) };
                    voxel.values = std::vector<float>(_valueData[0].size(), 0.0f);
                }
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

void VoxelBox::setBounds(const Bounds3D& bounds) {
    _bounds = bounds;
    voxelize();
}

void VoxelBox::setBoxSize(int xVoxels, int yVoxels, int zVoxels) {
    _xVoxels = xVoxels;
    _yVoxels = yVoxels;
    _zVoxels = zVoxels;
    _voxels.resize(_xVoxels * _yVoxels * _zVoxels);
    voxelize();
}

mv::Vector3f VoxelBox::getDims() const {
    return { static_cast<float>(_xVoxels), static_cast<float>(_yVoxels), static_cast<float>(_zVoxels) };
}

mv::Vector3f VoxelBox::getCenter() const {
    return { _xVoxels / 2.0f, _yVoxels / 2.0f, _zVoxels / 2.0f };
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
