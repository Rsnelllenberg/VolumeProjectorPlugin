#include <vector>
#include <cmath>

#include "Bounds3D.h"
#include "graphics/Vector3f.h"
#include "graphics/Texture.h"
#include <QOpenGLTexture>


struct Voxel {
    mv::Vector3f position;
    std::vector<float> values;
};

namespace mv {
    class Texture3D : public Texture
    {
    public:
        Texture3D() : Texture(GL_TEXTURE_3D) {}

        void initialize() {
            bind();
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            release();
        }

        void setData(int width, int height, int depth, std::vector<float> textureData) {
            glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, width, height, depth, 0, GL_RED, GL_FLOAT, textureData.data());
        }

        void setData(int width, int height, int depth, std::vector<float> textureData, int numDimensions) {
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, width, height, depth, 0, GL_RGBA, GL_FLOAT, textureData.data());
        }

    };
}

class VoxelBox{
public:
    void init(int xVoxels, int yVoxels, int zVoxels);

    void updateBounds();
    void voxelize();

    void setData(const std::vector<float>& spatialData, const std::vector<float>& valueData, int numValueDimensions);
    void setBoxSize(int xVoxels, int yVoxels, int zVoxels);

    const std::vector<Voxel>& getVoxels() const { return _voxels; }
    const Voxel& getVoxel(int x, int y, int z) const;
    mv::Texture3D& getVolumeTexture();
    size_t getBoxSize() const;
    mv::Vector3f getDims() const;
    mv::Vector3f getCenter() const; 
    bool hasData() const { return containsData; }

private:
    int _xVoxels, _yVoxels, _zVoxels;
    bool containsData = false;
    int _numValueDimensions = 0;

    Bounds3D _bounds;
    std::vector<Voxel> _voxels;
    std::vector<float> _spatialData;
    std::vector<float> _valueData;
    mv::Texture3D _volumeTexture;

    mv::Vector3f normalizePosition(const mv::Vector3f& pos) const;
    size_t getVoxelIndex(int x, int y, int z) const;
};
