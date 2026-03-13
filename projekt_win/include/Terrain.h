// include/Terrain.h
#ifndef TERRAIN_H
#define TERRAIN_H

#include <glm/glm.hpp>
#include <vector>
#include <GL/glew.h> // Include GLEW for OpenGL types like GLuint
#include "Shader.h"
class Terrain {
public:
    Terrain(int gridSize, float gridSpacing);
    ~Terrain();

    bool init(GLuint heightMapTextureID);
    void cleanup();

    glm::vec3 getVertex(int x, int z) const; // Get vertex position at grid index (x, z)
    float sampleHeightWorld(float worldX, float worldZ) const;
    float sampleGradientMagnitudeWorld(float worldX, float worldZ) const;
    float sampleDirectionalGradientWorld(float worldX, float worldZ, const glm::vec2& directionWorld) const;
    bool isInsideBoundsWorld(float worldX, float worldZ) const;

    int getGridSize() const { return gridSize; }
    float getGridSpacing() const { return gridSpacing; }
    GLuint getVAO() const { return vaoID; }
    GLuint getIndexCount() const { return indexCount; }
    glm::vec3 getNormal(int x, int z) const;
    glm::vec3 getHighestPoint() const { return highestPoint; }
    std::vector<glm::vec3> getHighestPeaks(int count, float minDistanceWorld) const;
private:
    int gridSize;
    float gridSpacing;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    GLuint vertexBufferID;
    GLuint normalBufferID;
    GLuint texCoordBufferID;
    GLuint indexBufferID;
    GLuint vaoID;
    unsigned int indexCount;
    glm::vec3 highestPoint;


    void generateGrid(GLuint heightMapTextureID); // Generate the initial grid of vertices
    void createBuffers(); // Create and populate VBOs and IBO
    void updateBuffers();   // Update VBO data (not needed for static terrain in this example, but good to have)
};

#endif // TERRAIN_H
