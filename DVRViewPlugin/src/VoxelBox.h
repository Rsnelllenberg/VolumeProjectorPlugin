//#include <vector>
//#include <cmath>
//
//#include "Bounds3D.h"
//#include "graphics/Vector3f.h"
//#include "graphics/Texture.h"
//#include <QOpenGLTexture>
//
//
//struct Voxel {
//    mv::Vector3f position;
//    std::vector<float> values;
//};
//
//class VoxelBox{
//public:
//    void init(int xVoxels, int yVoxels, int zVoxels);
//
//    void updateBounds();
//    //void voxelize();
//
//    mv::Vector3f findOptimalDimensions(int N, mv::Vector3f maxDims);
//
//    void setData(const std::vector<float>& spatialData, const std::vector<float>& valueData, int numValueDimensions);
//    void updateTexture(int numValueDimensions);
//    void setBoxSize(int xVoxels, int yVoxels, int zVoxels);
//
//    const std::vector<Voxel>& getVoxels() const { return _voxels; }
//    const Voxel& getVoxel(int x, int y, int z) const;
//    mv::Texture3D& getVolumeTexture();
//    mv::Vector3f getBrickLayout();
//    size_t getBoxSize() const;
//    mv::Vector3f getDims() const;
//    mv::Vector3f getCenter() const; 
//    bool hasData() const { return containsData; }
//
//private:
//    int _xVoxels, _yVoxels, _zVoxels;
//    bool containsData = false;
//    int _numValueDimensions = 0;
//    mv::Vector3f _brickLayout;
//
//    Bounds3D _bounds;
//    std::vector<Voxel> _voxels;
//    std::vector<float> _spatialData;
//    std::vector<float> _valueData;
//    mv::Texture3D _volumeTexture;
//    
//
//    mv::Vector3f normalizePosition(const mv::Vector3f& pos) const;
//    size_t getVoxelIndex(int x, int y, int z) const;
//};
