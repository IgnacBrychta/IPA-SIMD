/*
 * File:        Ocean.cpp
 * Author:      Tomas Goldmann
 * Date:        2025-03-23
 * Description: This class defines ocean (surface). The core functions of this class are optimized by the IPA Project 2025.
 *
 * Copyright (c) 2025, Brno University of Technology. All rights reserved.
 * Licensed under the MIT.
 */

#include "Ocean.h"
#include <cmath>
#include <glm/gtc/constants.hpp> // For pi

Ocean::Ocean(int gridSize) : time(0.0f),gridSize(gridSize), gridSpacing(1.0f), 
                             amplitude(0.8f), wavelength(10.0f), frequency(1.0f), // Adjusted amplitude slightly
                             direction(glm::vec2(1.0f, 0.0f)), phase(0.0f), seaLevelOffset(-2.0f)
{
    //gerstnerWaves.push_back({1.0f, 10.0f, 1.0f, glm::normalize(glm::vec2(1.0f, 0.0f)), 0.0f});
    //gerstnerWaves.push_back({0.3f, 5.0f, 2.0f, glm::normalize(glm::vec2(1.0f, 1.0f)), 0.0f});
    //gerstnerWaves.push_back({1.0f, 3.0f, 1.0f, glm::normalize(glm::vec2(1.0f, 0.5f)), 0.0f});
    //gerstnerWaves.push_back({0.3f, 5.0f, 2.0f, glm::normalize(glm::vec2(0.5f, 0.5f)), 0.0f});
    //gerstnerWaves.push_back({1.0f, 10.0f, 1.0f, glm::normalize(glm::vec2(1.0f, 0.0f)), 0.0f});
    //gerstnerWaves.push_back({0.5f, 2.0f, 3.0f, glm::normalize(glm::vec2(1.0f, 1.0f)), 0.0f});
    //gerstnerWaves.push_back({1.0f, 1.0f, 0.2f, glm::normalize(glm::vec2(0.7f, 0.2f)), 0.0f});
    //gerstnerWaves.push_back({0.5f, 1.2f, 2.0f, glm::normalize(glm::vec2(0.9f, 0.8f)), 0.0f});
    #if (0)
    gerstnerWaves.push_back({0.12f, 6.5f, 1.2f, glm::normalize(glm::vec2(1.0f, 0.8f)), 0.0f});
    gerstnerWaves.push_back({0.10f, 9.0f, 1.0f, glm::normalize(glm::vec2(-1.0f, 0.4f)), 1.0f});
    gerstnerWaves.push_back({0.08f, 13.0f, 0.8f, glm::normalize(glm::vec2(0.2f, -1.0f)), 2.2f});
    gerstnerWaves.push_back({0.07f, 16.0f, 0.6f, glm::normalize(glm::vec2(-0.6f, -1.0f)), 3.0f});
    gerstnerWaves.push_back({0.11f, 7.5f, 1.1f, glm::normalize(glm::vec2(0.0f, 1.0f)), 0.5f});
    #endif
    #if (0) 
    gerstnerWaves.push_back({0.18f, 7.0f, 0.9f, glm::normalize(glm::vec2(0.8f, 0.6f)), 0.0f});
    gerstnerWaves.push_back({0.15f, 10.0f, 0.8f, glm::normalize(glm::vec2(-0.9f, 0.4f)), 1.1f});
    gerstnerWaves.push_back({0.13f, 14.0f, 0.7f, glm::normalize(glm::vec2(0.3f, -1.0f)), 2.3f});
    gerstnerWaves.push_back({0.12f, 18.0f, 0.6f, glm::normalize(glm::vec2(-0.6f, -1.0f)), 3.0f});
    gerstnerWaves.push_back({0.16f, 8.5f, 0.85f, glm::normalize(glm::vec2(0.0f, 1.0f)), 0.7f});
    #endif
    #if (1)
    gerstnerWaves.push_back({0.3f, 5.0f, 2.0f, glm::normalize(glm::vec2(1.0f, 1.0f)), 0.0f});
    gerstnerWaves.push_back({0.2f, 8.0f, 1.5f, glm::normalize(glm::vec2(-1.0f, 0.5f)), 1.0f});
    gerstnerWaves.push_back({0.15f, 12.0f, 1.0f, glm::normalize(glm::vec2(0.3f, -1.0f)), 2.5f});
    gerstnerWaves.push_back({0.1f, 15.0f, 0.8f, glm::normalize(glm::vec2(-0.7f, -0.7f)), 3.7f});
    gerstnerWaves.push_back({0.25f, 7.0f, 1.8f, glm::normalize(glm::vec2(0.0f, 1.0f)), 0.8f});
    #endif

}



bool Ocean::init()
{
    generateGrid();
    createBuffers(); // Create VBOs and IBO

    return true;
}


void Ocean::update(float deltaTime)
{
    time += deltaTime;
    std::vector<glm::vec3> updatedVertices_vec = vertices;
    std::vector<glm::vec3> updatedNormals_vec(vertices.size());
    updateVertices(&updatedVertices_vec, &updatedNormals_vec, originalWorldX.data(), originalWorldZ.data(), gridSize, time);
    updateBuffers(updatedVertices_vec, updatedNormals_vec);
}

void Ocean::generateGrid()
{
    //std::cout << "Ocean::generateGrid - gridSize: " << gridSize << " " << vertices.size()<< std::endl;
    vertices.resize(gridSize * gridSize);
    // Resize original coordinate vectors
    originalWorldX.resize(gridSize * gridSize);
    originalWorldZ.resize(gridSize * gridSize);

    for (int x = 0; x < gridSize; ++x) {
        for (int z = 0; z < gridSize; ++z) {
            float worldX = (x - gridSize / 2.0f) * gridSpacing;
            float worldZ = (z - gridSize / 2.0f) * gridSpacing;
            vertices[x * gridSize + z] = glm::vec3(worldX, 0.0f, worldZ);

            // Store original world coordinates
            originalWorldX[x * gridSize + z] = worldX;
            originalWorldZ[x * gridSize + z] = worldZ;
        }
    }
}



void Ocean::updateVertices(std::vector<glm::vec3> * updatedVertices, std::vector<glm::vec3> * updatedNormals, float * originalWorldX_,  float * originalWorldZ_, int _grid_size, float time)
{

    for (int x = 0; x < _grid_size; ++x)
    {
        for (int z = 0; z < _grid_size; ++z)
        {

            glm::vec3 &vertex = (*updatedVertices)[x * _grid_size + z];  // Use reference to modify directly
            float originalX = originalWorldX_[x * _grid_size + z];
            float originalZ = originalWorldZ_[x * _grid_size + z];
            vertex.y = getWaveHeight(vertex.x, vertex.z, time);
            (*updatedNormals)[x * gridSize + z] = getWaveNormal(originalX, originalZ, time); // Calculate normal using original coords
        }
    }
}



float Ocean::getWaveHeight(float x, float z, float time) const {
    float totalHeight = 0.0f;
    for (const auto& wave : gerstnerWaves) {
        totalHeight += getGerstnerWaveHeight(wave, x, z, time);
    }
    return totalHeight + seaLevelOffset;
}

float Ocean::getGerstnerWaveHeight(const GerstnerWave& wave, float x, float z, float time) const {
    float k = 2.0f * glm::pi<float>() / wave.wavelength;
    float w = wave.speed * k;
    float dotProduct = glm::dot(wave.direction, glm::vec2(x, z));
    float periodicAmplitude = wave.amplitude * 0.5f * (1.0f + sin(2.0f * glm::pi<float>() * time / wave.wavelength));
    float waveHeightValue = periodicAmplitude * sin(k * dotProduct - w * time + wave.phase);

    if (fabs(x) < 0.5f && fabs(z) < 0.5f) {
        //std::cout << "  getGerstnerWaveHeight - time: " << time << ", periodicAmplitude: " << periodicAmplitude << ", waveHeightValue: " << waveHeightValue << std::endl;
    }
    /*
    if(c<8){
        float a = 2.0f * glm::pi<float>() * time / wave.wavelength;
        puts("");
        printf("2*pi*time/wave.wavelength: %f \n",a);
        printf("sin(2*pi*time/wave.wavelength): %f \n",sin(a));
        printf("k*dot: %f \n",k * dotProduct);
        printf("w*time: %f \n",w * time);
        printf("wave phase: %f \n",wave.phase);
        printf("k*dot - w*time + phase: %f \n",k * dotProduct-w*time+wave.phase);
        printf("sin(k*dot - w*time + phase: %f )\n",sin(k * dotProduct-w*time+wave.phase));
        printf("res: %f pariodic: %f dot: %f time: %f wave_x x: %f %f wave_y z: %f %f\n",waveHeightValue,periodicAmplitude,dotProduct,time,wave.direction.x,x,wave.direction.y,z);
        puts("");
        c++;
        if(c==7){
            puts("");
        }
    }
    */

    return waveHeightValue;
}

glm::vec3 Ocean::getGerstnerWaveDisplacement(const GerstnerWave& wave, float x, float z, float time) const {
    float k = 2.0f * glm::pi<float>() / wave.wavelength;
    float w = wave.speed * k;
    float dotProduct = glm::dot(wave.direction, glm::vec2(x, z));
    float cosTerm = cos(k * dotProduct - w * time + wave.phase);
    float periodicAmplitude = wave.amplitude * 0.5f * (1.0f + sin(2.0f * glm::pi<float>() * time / wave.wavelength));
    return glm::vec3(wave.direction.x * periodicAmplitude * cosTerm,
                     0.0f,
                     wave.direction.y * periodicAmplitude * cosTerm);
}

glm::vec3 Ocean::getWaveNormal(float x, float z, float time) const {
    glm::vec3 tangentX = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 tangentZ = glm::vec3(0.0f, 0.0f, 1.0f);

    for (const auto& wave : gerstnerWaves) {
        float k = 2.0f * glm::pi<float>() / wave.wavelength;
        float w = wave.speed * k;
        float dotProduct = glm::dot(wave.direction, glm::vec2(x, z));
        float sinTerm = sin(k * dotProduct - w * time + wave.phase);
        float cosTerm = cos(k * dotProduct - w * time + wave.phase);
        float periodicAmplitude = wave.amplitude * 0.5f * (1.0f + sin(2.0f * glm::pi<float>() * time /  wave.wavelength));
        float modulatedAmplitude = periodicAmplitude;
        // Calculate tangent vectors for EACH wave component and ACCUMULATE them directly
        tangentX += glm::vec3(
            -modulatedAmplitude * wave.direction.x * wave.direction.x * k * sinTerm, // dx_dx
             modulatedAmplitude * wave.direction.x * k * cosTerm,                 // dy_dx
            -modulatedAmplitude * wave.direction.x * wave.direction.y * k * sinTerm  // dz_dx
        );

        tangentZ += glm::vec3(
            -modulatedAmplitude * wave.direction.x * wave.direction.y * k * sinTerm, // dx_dz
             modulatedAmplitude * wave.direction.y * k * cosTerm,                 // dy_dz
            -modulatedAmplitude * wave.direction.y * wave.direction.y * k * sinTerm  // dz_dz
        );
        
    }
    /*
    auto cros = glm::cross(tangentZ, tangentX);
    if(c==0&&time!=0){
        puts("tangent:");
        c++;
        std::cout<<tangentX.x<<std::endl;
        std::cout<<tangentX.z<<std::endl;

        std::cout<<tangentZ.x<<std::endl;
        std::cout<<tangentZ.z<<std::endl;

        std::cout<<tangentX.y<<std::endl;
        std::cout<<tangentZ.y<<std::endl;
        puts("cross:");
        std::cout<<cros.x<<std::endl;
        std::cout<<cros.y<<std::endl;
        std::cout<<cros.z<<std::endl;
    }
    */
    return glm::normalize(glm::cross(tangentZ, tangentX));
}


void Ocean::updateBuffers(const std::vector<glm::vec3> &updatedVertices, const std::vector<glm::vec3> &updatedNormals)
{
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, updatedVertices.size() * sizeof(glm::vec3), updatedVertices.data()); // Update vertex positions

    glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, updatedNormals.size() * sizeof(glm::vec3), updatedNormals.data()); // Update normals
}

glm::vec3 Ocean::getVertex(int x, int z) const
{
    return vertices[x * gridSize + z];
}


void Ocean::setGridSize(int newGridSize)
{
    gridSize = newGridSize;
    generateGrid();   // Re-generate the grid with the new size

    std::vector<glm::vec3> updatedVertices = vertices; // Create a copy to update
    std::vector<glm::vec3> updatedNormals(vertices.size()); // Vector to store updated normals

    updateVertices(&updatedVertices, &updatedNormals, originalWorldX.data(), originalWorldZ.data(), gridSize, time);

    updateBuffers(updatedVertices, updatedNormals); 
    //updateVertices(); // Re-update vertices based on waves (optional, if needed immediately)
}

GLuint Ocean::getVAO() const
{
    return vaoID;
}

GLuint Ocean::getIndexCount() const
{
    return indexCount;
}

int Ocean::getGridIndex(int x, int z) const {
    return x * gridSize + z;
}

void Ocean::createBuffers()
{
    glGenVertexArrays(1, &vaoID);
    checkGLError("glGenVertexArrays"); // Check after glGenVertexArrays
    glBindVertexArray(vaoID);
    checkGLError("glBindVertexArray"); // Check after glBindVertexArray

    // Generate VBOs
    glGenBuffers(1, &vertexBufferID);
    checkGLError("glGenBuffers - vertexBufferID"); // Check after glGenBuffers
    glGenBuffers(1, &normalBufferID);
    checkGLError("glGenBuffers - normalBufferID"); // Check after glGenBuffers
    glGenBuffers(1, &texCoordBufferID);
    checkGLError("glGenBuffers - texCoordBufferID"); // Check after glGenBuffers
    glGenBuffers(1, &indexBufferID);
    checkGLError("glGenBuffers - indexBufferID"); // Check after glGenBuffers

    // 1. Vertex Positions VBO
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    checkGLError("glBindBuffer - vertexBufferID"); // Check after glBindBuffer
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);
    checkGLError("glBufferData - vertexBufferID"); // Check after glBufferData
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    checkGLError("glVertexAttribPointer - vertexBufferID"); // Check after glVertexAttribPointer
    glEnableVertexAttribArray(0);
    checkGLError("glEnableVertexAttribArray - location 0"); // Check after glEnableVertexAttribArray

    // 2. Vertex Normals VBO (initially flat normals - updated in updateVertices/updateBuffers)
    std::vector<glm::vec3> normals(vertices.size(), glm::vec3(0.0f, 1.0f, 0.0f)); // Initialize with flat normals
    glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
    checkGLError("glBindBuffer - normalBufferID"); // Check after glBindBuffer
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_DYNAMIC_DRAW);
    checkGLError("glBufferData - normalBufferID"); // Check after glBufferData
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    checkGLError("glVertexAttribPointer - normalBufferID"); // Check after glVertexAttribPointer
    glEnableVertexAttribArray(1);
    checkGLError("glEnableVertexAttribArray - location 1"); // Check after glEnableVertexAttribArray

    // 3. Texture Coordinates VBO
    std::vector<glm::vec2> texCoords(vertices.size());
    for (int x = 0; x < gridSize; ++x)
    {
        for (int z = 0; z < gridSize; ++z)
        {
            float texU = static_cast<float>(x) / 70.0f;
            float texV = static_cast<float>(z) / 70.0f;
            texCoords[x * gridSize + z] = glm::vec2(texU, texV);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, texCoordBufferID);
    checkGLError("glBindBuffer - texCoordBufferID"); // Check after glBindBuffer
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(glm::vec2), texCoords.data(), GL_STATIC_DRAW);
    checkGLError("glBufferData - texCoordBufferID"); // Check after glBufferData
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    checkGLError("glVertexAttribPointer - texCoordBufferID"); // Check after glVertexAttribPointer
    glEnableVertexAttribArray(2);
    checkGLError("glEnableVertexAttribArray - location 2"); // Check after glEnableVertexAttribArray

    // 4. Index Buffer Object (IBO) for Quads
    std::vector<unsigned int> indices;
    for (int x = 0; x < gridSize - 1; ++x)
    {
        for (int z = 0; z < gridSize - 1; ++z)
        {
            unsigned int v00 = x * gridSize + z;
            unsigned int v10 = (x + 1) * gridSize + z;
            unsigned int v11 = (x + 1) * gridSize + (z + 1);
            unsigned int v01 = x * gridSize + (z + 1);
            indices.insert(indices.end(), {v00, v10, v11, v01}); // Quad indices (counter-clockwise)
        }
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    checkGLError("glBindBuffer - indexBufferID"); // Check after glBindBuffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    checkGLError("glBufferData - indexBufferID"); // Check after glBufferData

    glBindVertexArray(0);                                   // Unbind VAO
    checkGLError("glBindVertexArray(0)");                   // Check after unbinding VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);                       // Unbind VBO - Optional, VAO unbinding often unbinds VBOs
    checkGLError("glBindBuffer(0) - ARRAY_BUFFER");         // Check after unbinding ARRAY_BUFFER
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);               // Unbind IBO - Optional, VAO unbinding often unbinds IBO
    checkGLError("glBindBuffer(0) - ELEMENT_ARRAY_BUFFER"); // Check after unbinding ELEMENT_ARRAY_BUFFER
    indexCount = indices.size();                            // Store index count for rendering
}

Ocean::~Ocean()
{
    cleanup(); // Call cleanup to release OpenGL resources
}

void Ocean::cleanup()
{
    // No dynamic memory allocation in this simple version
    // Release OpenGL resources (VBOs, IBO, VAO)
    glDeleteBuffers(1, &vertexBufferID);
    glDeleteBuffers(1, &normalBufferID);
    glDeleteBuffers(1, &texCoordBufferID);
    glDeleteBuffers(1, &indexBufferID);
    glDeleteVertexArrays(1, &vaoID);
    vertexBufferID = 0;
    normalBufferID = 0;
    texCoordBufferID = 0;
    indexBufferID = 0;
    vaoID = 0;
}
