#ifndef RENDERER_H
#define RENDERER_H

#include "math3d.h"
#include <vector>
#include <cstdint>
#include <string>

struct Triangle {
    Vec3 v0, v1, v2;
    Vec3 n0, n1, n2;
};

enum ShapeType {
    Cube,
    Sphere,
    Torus,
    Pyramid,
    MobiusStrip,
    CustomOBJ
};

enum class LightModel {
    Lambert,
    Phong
};

class Renderer {
public:
    int width, height;
    std::vector<uint8_t> framebuffer;
    std::vector<float> depthbuffer;

    static float ambientIntensity;
    static bool enableCulling;

    // Параметры камеры
    static float cameraDistance;
    static float cameraAngleX;
    static float cameraAngleY;

    // Флаг вращения
    static bool autoRotate;

    // Тип фигуры
    static ShapeType currentShape;

    // Флаг отключения culling для ленты Мёбиуса
    static bool disableCullingForMobius;
    
    // Модель освещения
    static LightModel currentLightModel;
    
    // Путь к файлу
    static std::string objFilePath;

    Renderer(int w, int h);

    void clear();
    void drawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, float z = 0.0f);
    void drawLine(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, float z0, float z1);
    void fillTriangle(const Triangle& t, uint8_t r, uint8_t g, uint8_t b);
    void drawTriangle(const Triangle& t, uint8_t r, uint8_t g, uint8_t b);
    void renderScene();
    void present();

    void setAmbientIntensity(float intensity);
    void setCullingEnabled(bool enabled);

    Vec3 project(const Vec3& v, float fov, float aspect, float near, float far);
    Vec3 computeLambert(const Vec3& normal, const Vec3& color, float diffuse);
    Vec3 computePhong(const Vec3& normal, const Vec3& viewDir, const Vec3& lightDir, const Vec3& color);
};

std::vector<Triangle> generateCube();
std::vector<Triangle> generateSphere(int stacks = 16, int slices = 16);
std::vector<Triangle> generateTorus(float radius = 1.0f, float tubeRadius = 0.3f, int stacks = 16, int slices = 16);
std::vector<Triangle> generatePyramid();
std::vector<Triangle> generateMobiusStrip(int segments = 64, int rings = 32);
std::vector<Triangle> loadOBJ(const std::string& path);


#endif
