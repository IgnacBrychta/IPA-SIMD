// Ocean.h
#ifndef OCEAN_H
#define OCEAN_H

#include <glm/glm.hpp>
#include <vector>
#include <GL/glew.h> // Include GLEW for OpenGL types like GLuint
#include "utils.h" // **Include utils.h to use checkGLError**
#include <immintrin.h>
#include <x86intrin.h>



#define ASM_TYPE CLEAR_ASM


#define INTRINSIC 1
#define CLEAR_ASM 2


// Structure to hold parameters for a single Gerstner wave component
struct GerstnerWave {
    float amplitude;    // Wave amplitude (height)
    float wavelength;   // Wavelength (distance between crests)
    float speed;        // Wave speed
    glm::vec2 direction; // Wave direction (normalized 2D vector in XZ plane)
    float phase;        // Phase offset
};
struct SimdSGerstnerWave {
    float amplitude;    // Wave amplitude (height)
    float wavelength;   // Wavelength (distance between crests)
    float speed;        // Wave speed
    float dir_x;
    float dir_y;
    float phase;        // Phase offset
    float k;        // k
    float w;        // w
};



class Ocean {
public:
    Ocean(int gridSize);
    ~Ocean();

    bool init();
    void cleanup();
    void update(float deltaTime);

    glm::vec3 getVertex(int x, int z) const;
    float getWaveHeight(float x, float z, float time) const;
    glm::vec3 getWaveNormal(float x, float z, float time) const; // Calculate wave normal
    float getSeaLevelOffset() const { return seaLevelOffset; }

    void setGridSize(int newGridSize); // Setter function
    int getGridSize() const { return gridSize; }
    float getGridSpacing() const { return gridSpacing; }
    GLuint getVAO() const;                                // Get the Vertex Array Object ID
    GLuint getIndexCount() const;                         // Get the number of indices for rendering
    float time;


private:
    int gridSize;
    float gridSpacing;
    std::vector<glm::vec3> vertices; // Store vertices for optimization (optional)
    std::vector<GerstnerWave> gerstnerWaves; // Vector to store multiple Gerstner wave components

    // Wave parameters (adjustable)
    float amplitude;
    float wavelength;
    float frequency;
    glm::vec2 direction; // Wave direction
    float phase; // Initial phase


    GLuint vertexBufferID;    // VBO ID for vertex positions
    GLuint normalBufferID;    // VBO ID for vertex normals
    GLuint texCoordBufferID;  // VBO ID for texture coordinates
    GLuint indexBufferID;     // IBO ID for indices
    GLuint vaoID;             // VAO ID (Vertex Array Object)
    unsigned int indexCount;    // Number of indices for rendering

    std::vector<float> originalWorldX;        // Vector to store original undisplaced World X coordinates
    std::vector<float> originalWorldZ;        // Vector to store original undisplaced World Z coordinates

    float baseAmplitude;  // Base (maximum) wave amplitude for periodic modulation
    float seaLevelOffset;


    void generateGrid();
    void createBuffers();                        // Create and populate VBOs and IBO
    void updateBuffers(const std::vector<glm::vec3>& updatedVertices, const std::vector<glm::vec3>& updatedNormals); // Update VBO data
    //void updateVertices(std::vector<glm::vec3> * updatedVertices, std::vector<glm::vec3> * updatedNormals, float time); // Update vertex Y positions based on wave function
    void updateVertices(std::vector<glm::vec3> * updatedVertices, std::vector<glm::vec3> * updatedNormals, float * originalWorldX_,  float * originalWorldZ_, int _grid_size, float time);
    int getGridIndex(int x, int z) const;         // Helper function to get 1D index from 2D grid indices
    float getGerstnerWaveHeight(const GerstnerWave& wave, float x, float z, float time) const; // Calculate height for a single Gerstner wave
    glm::vec3 getGerstnerWaveDisplacement(const GerstnerWave& wave, float x, float z, float time) const; // Calculate horizontal displacement for a Gerstner wave
};

#endif // OCEAN_H
