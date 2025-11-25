#include "renderer.h"
#include <algorithm>
#include <iostream>
#include <GL/gl.h>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

// === Определяем статические переменные ===
float Renderer::ambientIntensity = 0.2f;
bool Renderer::enableCulling = true;
float Renderer::cameraDistance = 5.0f;
float Renderer::cameraAngleX = 0.0f;
float Renderer::cameraAngleY = 0.0f;
bool Renderer::autoRotate = true;
ShapeType Renderer::currentShape = ShapeType::Cube;
bool Renderer::disableCullingForMobius = false;
std::string Renderer::objFilePath = "";
LightModel Renderer::currentLightModel = LightModel::Lambert;

uint8_t clamp(int val) {
    return val < 0 ? 0 : (val > 255 ? 255 : val);
}

Vec3 Renderer::computeLambert(const Vec3& normal, const Vec3& color, float diffuse) {
    Vec3 lightPos = {0, 5, -5}; // свет сверху и сзади
    float NdotL = std::max(0.0f, normal.normalize().dot((lightPos - Vec3(0,0,0)).normalize()));
    float intensity = Renderer::ambientIntensity + diffuse * NdotL;
    return {
        (float)clamp(color.x * intensity * 255),
        (float)clamp(color.y * intensity * 255),
        (float)clamp(color.z * intensity * 255)
    };
}

Vec3 Renderer::computePhong(const Vec3& normal, const Vec3& viewDir, const Vec3& lightDir, const Vec3& color) {
    // Diffuse
    float NdotL = std::max(0.0f, normal.dot(lightDir));
    Vec3 diffuse = color * (0.8f * NdotL);

    // Specular
    Vec3 reflectDir = (lightDir * -2.0f * normal.dot(lightDir)) + normal;
    float spec = std::pow(std::max(0.0f, viewDir.dot(reflectDir)), 32.0f); // 32 = shininess
    Vec3 specular = Vec3(1, 1, 1) * (0.5f * spec);

    // Ambient
    Vec3 ambient = color * Renderer::ambientIntensity;

    return {
        (float)clamp((int)((ambient.x + diffuse.x + specular.x) * 255)),
        (float)clamp((int)((ambient.y + diffuse.y + specular.y) * 255)),
        (float)clamp((int)((ambient.z + diffuse.z + specular.z) * 255))
    };
}

Renderer::Renderer(int w, int h) : width(w), height(h) {
    framebuffer.resize(width * height * 3, 0);
    depthbuffer.resize(width * height, 1e30f);
}

void Renderer::clear() {
    std::fill(framebuffer.begin(), framebuffer.end(), 0);
    std::fill(depthbuffer.begin(), depthbuffer.end(), 1e30f);
}

void Renderer::drawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, float z) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;

    int idx = (y * width + x);

    // Z-буфер: если текущий пиксель ближе — рисуем
    if (z < depthbuffer[idx]) {
        depthbuffer[idx] = z;

        int fbIdx = idx * 3;
        framebuffer[fbIdx + 0] = r;
        framebuffer[fbIdx + 1] = g;
        framebuffer[fbIdx + 2] = b;
    }
}

void Renderer::drawLine(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, float z0, float z1) {
    bool steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) { std::swap(x0, y0); std::swap(x1, y1); std::swap(z0, z1); }
    if (x0 > x1) { std::swap(x0, x1); std::swap(y0, y1); std::swap(z0, z1); }

    int dx = x1 - x0;
    int dy = abs(y1 - y0);
    int error = dx / 2;
    int ystep = (y0 < y1) ? 1 : -1;
    int y = y0;

    float dz = (z1 - z0) / (dx == 0 ? 1 : dx);
    float z = z0;

    for (int x = x0; x <= x1; x++) {
        if (steep) drawPixel(y, x, r, g, b, z); else drawPixel(x, y, r, g, b, z);
        error -= dy;
        if (error < 0) { y += ystep; error += dx; }
        z += dz;
    }
}

void Renderer::fillTriangle(const Triangle& t, uint8_t r, uint8_t g, uint8_t b) {
    // Определяем границы
    int minX = (int)std::min({t.v0.x, t.v1.x, t.v2.x});
    int maxX = (int)std::max({t.v0.x, t.v1.x, t.v2.x});
    int minY = (int)std::min({t.v0.y, t.v1.y, t.v2.y});
    int maxY = (int)std::max({t.v0.y, t.v1.y, t.v2.y});

    // Среднее Z
    float avgZ = (t.v0.z + t.v1.z + t.v2.z) / 3.0f;

    auto sign = [](float x1, float y1, float x2, float y2, float x3, float y3) -> float {
        return (x1 - x3) * (y2 - y3) - (x2 - x3) * (y1 - y3);
    };

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            if (x < 0 || x >= width || y < 0 || y >= height) continue;

            float d1 = sign(x, y, t.v0.x, t.v0.y, t.v1.x, t.v1.y);
            float d2 = sign(x, y, t.v1.x, t.v1.y, t.v2.x, t.v2.y);
            float d3 = sign(x, y, t.v2.x, t.v2.y, t.v0.x, t.v0.y);

            bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

            if (!(has_neg && has_pos)) {
                drawPixel(x, y, r, g, b, avgZ);
            }
        }
    }
}

void Renderer::drawTriangle(const Triangle& t, uint8_t r, uint8_t g, uint8_t b) {
    if (enableCulling && !disableCullingForMobius) {
        Vec3 v0 = t.v0;
        Vec3 v1 = t.v1;
        Vec3 v2 = t.v2;

        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        float z = edge1.x * edge2.y - edge1.y * edge2.x;

        // Отбрасываем грани
        if (z < 0) return;
    }

    fillTriangle(t, r, g, b);
}

std::vector<Triangle> generateCube() {
    std::vector<Triangle> triangles;

    // Позиции вершин куба (2x2x2, центр в (0,0,0))
    std::vector<Vec3> vertices = {
        {-1, -1, -1}, // 0
        { 1, -1, -1}, // 1
        { 1,  1, -1}, // 2
        {-1,  1, -1}, // 3
        {-1, -1,  1}, // 4
        { 1, -1,  1}, // 5
        { 1,  1,  1}, // 6
        {-1,  1,  1}  // 7
    };

    // Грани куба (каждая грань — 2 треугольника)
    std::vector<int> indices = {
        // Back face
        0, 1, 2, 0, 2, 3,
        // Front face
        4, 6, 5, 4, 7, 6,
        // Left face
        4, 3, 7, 4, 0, 3,
        // Right face
        1, 6, 2, 1, 5, 6,
        // Bottom face
        4, 5, 1, 4, 1, 0,
        // Top face
        3, 2, 6, 3, 6, 7
    };

    for (size_t i = 0; i < indices.size(); i += 3) {
        int i0 = indices[i];
        int i1 = indices[i + 1];
        int i2 = indices[i + 2];

        Vec3 v0 = vertices[i0];
        Vec3 v1 = vertices[i1];
        Vec3 v2 = vertices[i2];

        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        Vec3 normal = edge1.cross(edge2).normalize();

        Triangle t = {v0, v1, v2, normal, normal, normal};
        triangles.push_back(t);
    }

    return triangles;
}

std::vector<Triangle> generateSphere(int stacks, int slices) {
    std::vector<Triangle> triangles;
    std::vector<Vec3> vertices;

    for (int i = 0; i <= stacks; i++) {
        float phi = M_PI * i / stacks; // от 0 до π
        for (int j = 0; j <= slices; j++) {
            float theta = 2 * M_PI * j / slices; // от 0 до 2π

            float x = sin(phi) * cos(theta);
            float y = cos(phi);
            float z = sin(phi) * sin(theta);

            vertices.push_back({x, y, z});
        }
    }

    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;

            // Первый треугольник
            if (i < stacks - 1) {
                Vec3 v0 = vertices[first];
                Vec3 v1 = vertices[second];
                Vec3 v2 = vertices[second + 1];
                Vec3 normal = (v1 - v0).cross(v2 - v0).normalize();
                triangles.push_back({v0, v1, v2, normal, normal, normal});
            }

            // Второй треугольник
            if (i > 0) {
                Vec3 v0 = vertices[first];
                Vec3 v1 = vertices[second + 1];
                Vec3 v2 = vertices[first + 1];
                Vec3 normal = (v1 - v0).cross(v2 - v0).normalize();
                triangles.push_back({v0, v1, v2, normal, normal, normal});
            }
        }
    }

    return triangles;
}

std::vector<Triangle> generateTorus(float radius, float tubeRadius, int stacks, int slices) {
    std::vector<Triangle> triangles;
    std::vector<Vec3> vertices;

    for (int i = 0; i <= stacks; i++) {
        float theta = 2 * M_PI * i / stacks;
        for (int j = 0; j <= slices; j++) {
            float phi = 2 * M_PI * j / slices;

            float x = (radius + tubeRadius * cos(phi)) * cos(theta);
            float y = (radius + tubeRadius * cos(phi)) * sin(theta);
            float z = tubeRadius * sin(phi);

            vertices.push_back({x, y, z});
        }
    }

    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            int first = i * (slices + 1) + j;
            int second = first + slices + 1;

            Vec3 v0 = vertices[first];
            Vec3 v1 = vertices[second];
            Vec3 v2 = vertices[second + 1];
            Vec3 v3 = vertices[first + 1];

            Vec3 normal = (v1 - v0).cross(v2 - v0).normalize();
            triangles.push_back({v0, v2, v1, normal, normal, normal});
            triangles.push_back({v0, v3, v2, normal, normal, normal});
        }
    }

    return triangles;
}

std::vector<Triangle> generatePyramid() {
    std::vector<Triangle> triangles;

    // Основание: квадрат (4 вершины)
    Vec3 base[4] = {
        {-1, -1, -1}, // 0
        { 1, -1, -1}, // 1
        { 1, -1,  1}, // 2
        {-1, -1,  1}  // 3
    };

    // Вершина пирамиды
    Vec3 top = {0, 1, 0};

    // Грани пирамиды (4 треугольника)
    for (int i = 0; i < 4; i++) {
        int next = (i + 1) % 4;

        Vec3 v0 = base[i];
        Vec3 v1 = base[next];
        Vec3 v2 = top;

        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        Vec3 normal = edge1.cross(edge2).normalize();

        triangles.push_back({v0, v1, v2, normal, normal, normal});
    }

    // === Основание (2 треугольника, нормаль вверх) ===
    Vec3 n_bottom = {0, 1, 0}; // нормаль вверх
    triangles.push_back({base[0], base[3], base[1], n_bottom, n_bottom, n_bottom});
    triangles.push_back({base[1], base[3], base[2], n_bottom, n_bottom, n_bottom});

    return triangles;
}

std::vector<Triangle> generateMobiusStrip(int segments, int rings) {
    std::vector<Triangle> triangles;
    std::vector<Vec3> vertices;

    float width = 0.5f; // ширина ленты

    for (int i = 0; i <= rings; i++) {
        float u = (float)i / rings * 2.0f * M_PI; // от 0 до 2π
        for (int j = 0; j <= segments; j++) {
            float v = (float)j / segments * width * 2.0f - width; // от -width до +width

            // параметрическая формула ленты Мёбиуса
            float x = (1 + v * cos(u / 2)) * cos(u);
            float y = (1 + v * cos(u / 2)) * sin(u);
            float z = v * sin(u / 2);

            vertices.push_back({x, y, z});
        }
    }

    for (int i = 0; i < rings; i++) {
        for (int j = 0; j < segments; j++) {
            int first = i * (segments + 1) + j;
            int second = first + (segments + 1);

            Vec3 v0 = vertices[first];
            Vec3 v1 = vertices[second];
            Vec3 v2 = vertices[second + 1];
            Vec3 v3 = vertices[first + 1];

            Vec3 normal = (v1 - v0).cross(v2 - v0).normalize();
            triangles.push_back({v0, v1, v2, normal, normal, normal});
            triangles.push_back({v0, v2, v3, normal, normal, normal});
        }
    }

    return triangles;
}

std::vector<Triangle> loadOBJ(const std::string& path) {
    std::vector<Triangle> triangles;
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << path << std::endl;
        return triangles;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            vertices.push_back({x, y, z});
        }
        else if (prefix == "vn") {
            float x, y, z;
            iss >> x >> y >> z;
            normals.push_back({x, y, z});
        }
        else if (prefix == "f") {
            std::string vertex;
            std::vector<int> face;
            std::vector<int> normalIndices; // добавляем для нормалей
            while (iss >> vertex) {
                std::replace(vertex.begin(), vertex.end(), '/', ' ');
                std::istringstream viss(vertex);
                int vi, ti = 0, ni = 0;
                viss >> vi >> ti >> ni;

                if (vi < 0) vi = vertices.size() + vi + 1;
                if (ni < 0) ni = normals.size() + ni + 1;

                face.push_back(vi - 1);
                normalIndices.push_back(ni - 1);
            }

            // Триангуляция полигона (если > 3 вершин)
            for (size_t i = 2; i < face.size(); i++) {
                int v0 = face[0];
                int v1 = face[i - 1];
                int v2 = face[i];

                int n0 = normalIndices[0];
                int n1 = normalIndices[i - 1];
                int n2 = normalIndices[i];

                Vec3 n = {0, 0, 0};
                if (n0 >= 0 && n1 >= 0 && n2 >= 0 && n0 < (int)normals.size()) {
                    // Используем нормали из файла
                    n = normals[n0];
                } else {
                    // Вычисляем нормаль треугольника
                    Vec3 edge1 = vertices[v1] - vertices[v0];
                    Vec3 edge2 = vertices[v2] - vertices[v0];
                    n = edge1.cross(edge2).normalize();
                }

                triangles.push_back({vertices[v2], vertices[v1], vertices[v0], n, n, n});
            }
        }
        // Игнорируем все остальные строки (mtllib, usemtl, o, g и т.д.)
    }

    return triangles;
}

Vec3 Renderer::project(const Vec3& v, float fov, float aspect, float near, float far) {
    float scale = 1.0f / tan(fov / 2.0f);
    float z = v.z;
    if (z == 0) z = 0.001f; // избегаем деления на 0

    float x = v.x * scale / aspect;
    float y = v.y * scale;

    // Проецируем в 2D
    x = (x / z) * width / 2.0f + width / 2.0f;
    y = (-y / z) * height / 2.0f + height / 2.0f; // инвертируем Y

    return {x, y, z};
}

void Renderer::renderScene() {
    clear();

    static float cubeAngle = 0.0f;
    if (autoRotate) {
        cubeAngle += 0.01f;
    }

    // Камера
    float camX = cameraDistance * cos(cameraAngleX) * sin(cameraAngleY);
    float camY = cameraDistance * sin(cameraAngleX);
    float camZ = cameraDistance * cos(cameraAngleX) * cos(cameraAngleY);
    Vec3 cameraPos = {camX, camY, camZ};

    Vec3 target = {0, 0, 0};
    Vec3 forward = (target - cameraPos).normalize();
    Vec3 up = {0, 1, 0};
    Vec3 right = forward.cross(up).normalize();
    up = right.cross(forward).normalize();

    std::vector<Triangle> triangles;

    switch (currentShape) {
        case Cube:
            triangles = generateCube();
            disableCullingForMobius = false;
            break;
        case Sphere:
            triangles = generateSphere();
            disableCullingForMobius = false;
            break;
        case Torus:
            triangles = generateTorus();
            disableCullingForMobius = false;
            break;
        case Pyramid:
            triangles = generatePyramid();
            disableCullingForMobius = false;
            break;
        case MobiusStrip:
            triangles = generateMobiusStrip();
            disableCullingForMobius = true;
            break;
        case CustomOBJ:
            if (!objFilePath.empty()) {
                triangles = loadOBJ(objFilePath);
                disableCullingForMobius = false;
            } else {
                triangles = generateCube(); // fallback
            }
            break;
    }

    for (auto& t : triangles) {
        // Вращение вершин и нормалей
        Vec3 v0 = rotateY(rotateX(t.v0, cubeAngle), cubeAngle);
        Vec3 v1 = rotateY(rotateX(t.v1, cubeAngle), cubeAngle);
        Vec3 v2 = rotateY(rotateX(t.v2, cubeAngle), cubeAngle);

        Vec3 n0 = rotateY(rotateX(t.n0, cubeAngle), cubeAngle).normalize();
        Vec3 n1 = rotateY(rotateX(t.n1, cubeAngle), cubeAngle).normalize();
        Vec3 n2 = rotateY(rotateX(t.n2, cubeAngle), cubeAngle).normalize();

        // Масштабирование (для сферы и тора)
        v0 = v0 * 0.5f;
        v1 = v1 * 0.5f;
        v2 = v2 * 0.5f;

        // Переводим в систему координат камеры
        v0 = v0 - cameraPos;
        v1 = v1 - cameraPos;
        v2 = v2 - cameraPos;

        Vec3 v0_cam = { v0.dot(right), v0.dot(up), v0.dot(forward) };
        Vec3 v1_cam = { v1.dot(right), v1.dot(up), v1.dot(forward) };
        Vec3 v2_cam = { v2.dot(right), v2.dot(up), v2.dot(forward) };

        // Нормаль грани (усреднённая, в мировых координатах, после вращения)
        Vec3 faceNormal_world = (n0 + n1 + n2).normalize();

        // Вращаем нормаль в систему координат камеры
        Vec3 faceNormal_cam = {
            faceNormal_world.dot(right),
            faceNormal_world.dot(up),
            faceNormal_world.dot(forward)
        };
        faceNormal_cam = faceNormal_cam.normalize();

        // Освещение
        Vec3 baseColor = {1.0f, 1.0f, 0.0f};

        // Направление света
        Vec3 lightDir = (Vec3(0, 5, -5) - v0_cam).normalize();
        Vec3 viewDir = (Vec3(0, 0, 0) - v0_cam).normalize();

        Vec3 litColor;

        switch (currentLightModel) {
            case LightModel::Lambert:
                litColor = computeLambert(faceNormal_cam, baseColor, 0.8f);
                break;
            case LightModel::Phong:
                litColor = computePhong(faceNormal_cam, viewDir, lightDir, baseColor);
                break;
        }

        uint8_t r = (uint8_t)litColor.x;
        uint8_t g = (uint8_t)litColor.y;
        uint8_t b = (uint8_t)litColor.z;

        // Проекция
        Vec3 p0 = project(v0_cam, 1.0f, (float)width / height, 0.1f, 100.0f);
        Vec3 p1 = project(v1_cam, 1.0f, (float)width / height, 0.1f, 100.0f);
        Vec3 p2 = project(v2_cam, 1.0f, (float)width / height, 0.1f, 100.0f);

        Triangle projected = {p0, p1, p2, p0, p1, p2};
        drawTriangle(projected, r, g, b);
    }
}

void Renderer::present() {

    glRasterPos2f(-1, -1);
    glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, framebuffer.data());

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "OpenGL error in glDrawPixels: " << err << "\n";
    }
}

void Renderer::setAmbientIntensity(float intensity) {
    ambientIntensity = intensity;
}

void Renderer::setCullingEnabled(bool enabled) {
    enableCulling = enabled;
}
