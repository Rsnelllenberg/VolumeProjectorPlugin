#ifndef VOXELBOX_H
#define VOXELBOX_H

#include <vector>
#include <cmath>
#include "Bounds3D.h"
#include "graphics/Vector3f.h"

struct Voxel {
    mv::Vector3f position;
    std::vector<float> values;
};

class VoxelBox {
public:
    VoxelBox(int xVoxels, int yVoxels, int zVoxels, const Bounds3D& bounds);

    void setData(const std::vector<mv::Vector3f>& spatialData, const std::vector<std::vector<float>>& valueData);
    void voxelize();
    const std::vector<Voxel>& getVoxels() const { return _voxels; }
    const Voxel& getVoxel(int x, int y, int z) const;
    size_t getBoxSize() const;
    void setBounds(const Bounds3D& bounds);
    void setBoxSize(int xVoxels, int yVoxels, int zVoxels);
    mv::Vector3f getDims() const;

private:
    int _xVoxels, _yVoxels, _zVoxels;
    Bounds3D _bounds;
    std::vector<Voxel> _voxels;
    std::vector<mv::Vector3f> _spatialData;
    std::vector<std::vector<float>> _valueData;

    mv::Vector3f normalizePosition(const mv::Vector3f& pos) const;
    size_t getVoxelIndex(int x, int y, int z) const;
};

#endif // VOXELBOX_H
