#ifndef BOAT_H
#define BOAT_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include "Input.h"
#include "Ocean.h"
#include "Terrain.h"
#include <string>
#include <tiny_obj_loader.h> // Include tinyobjloader

class Boat {
public:
    Boat();
    ~Boat();

    bool init(const char* modelPath, const char* texturePath); // Pass model and texture paths
    void cleanup();
    void update(const Input& input, const Ocean& ocean, const Terrain& terrain, float deltaTime);

    glm::vec3 getPosition() const { return position; }
    glm::quat getRotation() const { return rotation; }
    float getSpeed() const { return speed; }
    bool hasTerrainCollision() const { return lastCollisionWithTerrain; }
    bool canDisembark() const { return lastCollisionAllowsDisembark; }
    glm::vec3 getDisembarkPosition() const { return lastCollisionPoint; }
    void clearCollisionState();

    glm::vec3 getBoundingBoxMin() const { return boundingBoxMin; } // **Getter for boundingBoxMin**
    glm::vec3 getBoundingBoxMax() const { return boundingBoxMax; } // **Getter for boundingBoxMax**

    // Getters for model data to pass to Renderer
    const std::vector<glm::vec3>& getVertices() const { return vertices; }
    const std::vector<glm::vec3>& getNormals() const { return normals; }
    const std::vector<glm::vec2>& getTexCoords() const { return texCoords; }
    const std::string& getTexturePath() const { return boatTexturePath; } // Getter for texture path
    const std::vector<tinyobj::material_t>& getMaterials() const { return materials; } // Getter for materials
    const std::vector<int>& getMaterialIndices() const { return materialIndices; } // Getter for materials

    // New: Getter and Setter for boatScale
    float getScale() const { return boatScale; }
    void setScale(float scale) { boatScale = scale; }


private:
    glm::vec3 position;
    glm::quat rotation;
    float speed;
    float steeringSpeed;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<int> materialIndices; // New: Store material indices per vertex
    std::vector<tinyobj::material_t> materials; // New: Store materials


    void handleInput(const Input& input, const Ocean& ocean, const Terrain& terrain, float deltaTime);
    void applyWaveMotion(const Ocean& ocean);
    bool collidesWithTerrain(const Terrain& terrain, const Ocean& ocean, const glm::vec3& candidatePosition, glm::vec3* collisionPoint) const;
    bool isGradientLowForDisembark(const Terrain& terrain, const glm::vec3& collisionPoint) const;
    float getCollisionRadius() const;
    bool loadModel(const char* path); // Function to load OBJ model
    std::string boatTexturePath; // Store texture path for Renderer to access
    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
    int getGridIndex(int x, int z) const;         // Helper function to get 1D index from 2D grid indices

    float boatScale;
    bool speedUpKeyWasDown;
    bool speedDownKeyWasDown;
    bool lastCollisionWithTerrain;
    bool lastCollisionAllowsDisembark;
    glm::vec3 lastCollisionPoint;


};

#endif // BOAT_H
