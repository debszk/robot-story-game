#include <GL/freeglut.h>
#include <GL/glu.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <vector>

struct Vec3
{
    float x, y, z;
};

struct RobotState
{
    float x, y, z;

    float pathT;
    float speed;
    float heading;

    float animationTime;

    bool walking;
    bool waving;

    int behaviorState;

    float scale;
    float pathOffset;

    float interactionCooldown;
};

struct Particle
{
    float x, y, z;
    float vx, vy, vz;
    float life;
    float maxLife;
};

void drawCylinderY(float radius, float height);
void drawCylinderX(float radius, float height);

bool isPaused = true;
int lightingMode = 3;
float lightAngle = 0.0f;
float robotRotationY = 0.0f;

float danceTimer = 0.0f;

float camX = 0, camY = 0, camZ = 0;
int followedRobot = 0;
float timeOfDay = 0.3f;

int currentView = 1;
bool usePerspective = true;
bool interactionsEnabled = true;
bool followCamera = false;
bool splitScreen = false;
float animationSpeed = 0.05f;

float waterLevel = -3.4f;

bool lightningActive = false;
float lightningTimer = 0.0f;

bool texturesEnabled = true;
bool showGrid = true;
bool showLabels = false;
bool showStars = true;
bool rescueMissionComplete = false;

bool storyMode = true;

enum StoryState
{
    STORY_START_POS,        
    STORY_MOVE_TO_CENTER,   
    STORY_LIE_DOWN,         
    STORY_LIGHTNING_STRIKE, 
    STORY_SPARKS,           
    STORY_REACT_TO_FALL,   
    STORY_RUN_AWAY,         
    STORY_DONE              
};

StoryState storyState = STORY_START_POS;

float storyTimer = 0.0f;

float robotAFallAngle = 0.0f;
float robotAFallOffsetY = 0.0f;

bool sparksSpawned = false;

float pondX = 0.0f;
float pondZ = 0.0f;

Vec3 startA = { -18.0f, 0, -18.0f };
Vec3 startB = { 18.0f, 0, -18.0f };
Vec3 startC = { -12.0f, 0, 15.0f };

Vec3 semiA = { -3.5f, 0, -2.0f };
Vec3 semiB = { 0.0f, 0, -1.5f };
Vec3 semiC = { 3.5f, 0, -2.0f };

GLuint robotTexture = 0;
GLuint floorTexture = 0;
GLuint skyTexture = 0;
GLuint pathTexture = 0;

std::vector<RobotState> robots;
std::vector<Particle> particles;
std::vector<RobotState> savedRobots;
bool specialDanceMode = false;

Vec3 ballPos;
int ballOwner = 0;
float ballT = 0.0f;
bool ballMoving = false;

const float PI = 3.1415926535f;
const float ROBOT_ROOT_ABOVE_GROUND = 3.6f;

float orbitYaw = 45.0f;
float orbitPitch = 28.0f;
float orbitDistance = 35.0f;
bool mouseDragging = false;
int lastMouseX = 0;
int lastMouseY = 0;

bool randomWalkMode = false;

StoryState savedStoryState;
float savedStoryTimer;
float savedFallAngle;
float savedFallOffsetY;
bool savedLightning;
float savedLightningTimer;

float clamp01(float t)
{
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 1.0f;
    return t;
}

Vec3 makeVec3(float x, float y, float z)
{
    Vec3 v = { x, y, z };
    return v;
}

Vec3 add(Vec3 a, Vec3 b)
{
    return makeVec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3 sub(Vec3 a, Vec3 b)
{
    return makeVec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3 mul(Vec3 a, float s)
{
    return makeVec3(a.x * s, a.y * s, a.z * s);
}

Vec3 normalizeVec(Vec3 v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 0.0001f) return makeVec3(0.0f, 1.0f, 0.0f);
    return makeVec3(v.x / len, v.y / len, v.z / len);
}

Vec3 crossVec(Vec3 a, Vec3 b)
{
    return makeVec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

GLuint createCheckerTexture(int size, unsigned char r1, unsigned char g1, unsigned char b1, unsigned char r2, unsigned char g2, unsigned char b2)
{
    unsigned char* data = new unsigned char[size * size * 3];

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int checker = ((x / 8) + (y / 8)) % 2;
            int index = (y * size + x) * 3;

            if (checker == 0)
            {
                data[index] = r1;
                data[index + 1] = g1;
                data[index + 2] = b1;
            }
            else
            {
                data[index] = r2;
                data[index + 1] = g2;
                data[index + 2] = b2;
            }
        }
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    delete[] data;
    return texture;
}

GLuint createGradientTexture(int w, int h)
{
    unsigned char* data = new unsigned char[w * h * 3];

    for (int y = 0; y < h; y++)
    {
        float t = (float)y / (float)(h - 1);
        for (int x = 0; x < w; x++)
        {
            int index = (y * w + x) * 3;
            data[index] = (unsigned char)(80 + 100 * t);
            data[index + 1] = (unsigned char)(120 + 100 * t);
            data[index + 2] = (unsigned char)(180 + 70 * t);
        }
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    delete[] data;
    return texture;
}

GLuint loadTexture(const char* filename)
{
    FILE* file = nullptr;
    fopen_s(&file, filename, "rb");

    if (!file)
    {
        printf("Texture not found: %s\n", filename);
        return 0;
    }

    unsigned char header[54];

    if (fread(header, 1, 54, file) != 54)
    {
        fclose(file);
        return 0;
    }

    if (header[0] != 'B' || header[1] != 'M')
    {
        fclose(file);
        return 0;
    }

    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
    short bitsPerPixel = *(short*)&header[28];

    if (bitsPerPixel != 24 || width <= 0 || height <= 0)
    {
        fclose(file);
        return 0;
    }

    int rowPadded = (width * 3 + 3) & (~3);

    unsigned char* data = new unsigned char[width * height * 3];
    unsigned char* row = new unsigned char[rowPadded];

    for (int y = 0; y < height; y++)
    {
        size_t bytesRead = fread(row, 1, rowPadded, file);
        if (bytesRead < (size_t)rowPadded) break;

        for (int x = 0; x < width; x++)
        {
            int src = x * 3;
            int dst = ((height - 1 - y) * width + x) * 3;
            data[dst] = row[src + 2];
            data[dst + 1] = row[src + 1];
            data[dst + 2] = row[src];
        }
    }

    delete[] row;
    fclose(file);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    delete[] data;
    return texture;
}

float surfaceHeight(float x, float z)
{
    float bigWaves =
        1.5f * sinf(0.12f * x) +
        1.2f * cosf(0.12f * z);

    float hills =
        0.8f * sinf(0.25f * x + 0.3f * z) +
        0.6f * cosf(0.3f * z - 0.2f * x);

    float smallDetail =
        0.25f * sinf(0.7f * x) +
        0.25f * cosf(0.6f * z);

    return bigWaves + hills + smallDetail;
}

float getGroundY(float x, float z)
{
    return surfaceHeight(x, z) - 1.5f;
}

Vec3 surfaceNormal(float x, float z)
{
    float e = 0.1f;
    float hL = surfaceHeight(x - e, z);
    float hR = surfaceHeight(x + e, z);
    float hD = surfaceHeight(x, z - e);
    float hU = surfaceHeight(x, z + e);

    Vec3 dx = makeVec3(2.0f * e, hR - hL, 0.0f);
    Vec3 dz = makeVec3(0.0f, hU - hD, 2.0f * e);
    return normalizeVec(crossVec(dz, dx));
}

Vec3 bezierPoint(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, float t)
{
    t = clamp01(t);
    float u = 1.0f - t;
    float b0 = u * u * u;
    float b1 = 3.0f * u * u * t;
    float b2 = 3.0f * u * t * t;
    float b3 = t * t * t;

    Vec3 p;
    p.x = b0 * p0.x + b1 * p1.x + b2 * p2.x + b3 * p3.x;
    p.z = b0 * p0.z + b1 * p1.z + b2 * p2.z + b3 * p3.z;
    p.y = surfaceHeight(p.x, p.z) - 1.5f;
    return p;
}

Vec3 evaluatePath(int pathId, float t)
{
    if (t > 1.0f) t -= floorf(t);
    if (t < 0.0f) t += 1.0f;

    if (pathId == 0)
    {
        return bezierPoint(
            makeVec3(-6.0f, 0, -12.0f),
            makeVec3(-18.0f, 0, 5.0f),
            makeVec3(18.0f, 0, -5.0f),
            makeVec3(6.0f, 0, 12.0f),
            t
        );
    }
    else if (pathId == 1)
    {
        return bezierPoint(
            makeVec3(-4.5f, 0, -13.0f),
            makeVec3(-20.0f, 0, 0.0f),
            makeVec3(20.0f, 0, 10.0f),
            makeVec3(4.5f, 0, 13.0f),
            t
        );
    }
    else
    {
        return bezierPoint(
            makeVec3(4.5f, 0, -13.0f),
            makeVec3(20.0f, 0, -2.0f),
            makeVec3(-20.0f, 0, 8.0f),
            makeVec3(-4.5f, 0, 13.0f),
            t
        );
    }
}

Vec3 evaluatePathTangent(int pathId, float t)
{
    Vec3 p1 = evaluatePath(pathId, t);
    Vec3 p2 = evaluatePath(pathId, t + 0.01f);
    return normalizeVec(sub(p2, p1));
}

Vec3 moveTowards(Vec3 current, Vec3 target, float speed)
{
    Vec3 dir = sub(target, current);
    float dist = sqrtf(dir.x * dir.x + dir.z * dir.z);

    if (dist < 0.001f)
        return target;

    dir.x /= dist;
    dir.z /= dist;

    current.x += dir.x * speed;
    current.z += dir.z * speed;

    return current;
}

void setMatteMaterial()
{
    GLfloat specular[] = { 0.05f, 0.05f, 0.05f, 1.0f };
    GLfloat shininess[] = { 8.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

void setPlasticMaterial()
{
    GLfloat specular[] = { 0.65f, 0.65f, 0.65f, 1.0f };
    GLfloat shininess[] = { 70.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

void setMetalMaterial()
{
    GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat shininess[] = { 120.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

void applyRobotMaterial(int index)
{
    if (index == 0) setPlasticMaterial();
    else if (index == 1) setMatteMaterial();
    else setMetalMaterial();
}

Vec3 getSunPosition()
{
    float t = timeOfDay;

    float angle = (t - 0.5f) * PI;

    float radius = 18.0f;

    return {
    -radius * cosf(angle),
    radius * sinf(angle),
    -30.0f
    };
}

Vec3 getMoonPosition()
{
    float t = timeOfDay;

    float angle = t * PI;

    float radius = 18.0f;

    return {
        radius * sinf(angle),
        radius * cosf(angle) + 5.0f,
        -50.0f
    };
}

float daylightAmount()
{
    Vec3 sun = getSunPosition();

    float t = (sun.y + 5.0f) / 20.0f;

    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    return t;
}

void setupLighting(int mode)
{
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHT2);

    GLfloat globalAmbient[] = { 0.45f, 0.45f, 0.50f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    if (mode == 1)
    {
        glEnable(GL_LIGHT0);
        GLfloat dir[] = { -0.6f, -1.0f, -0.4f, 0.0f };
        GLfloat amb[] = { 0.25f, 0.25f, 0.25f, 1.0f };
        GLfloat diff[] = { 0.85f, 0.85f, 0.85f, 1.0f };
        GLfloat spec[] = { 0.7f, 0.7f, 0.7f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, dir);
        glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
    }
    else if (mode == 2)
    {
        glEnable(GL_LIGHT0);
        GLfloat pos[] = { 4.0f, 8.0f, 4.0f, 1.0f };
        GLfloat amb[] = { 0.15f, 0.15f, 0.15f, 1.0f };
        GLfloat diff[] = { 1.0f, 0.95f, 0.80f, 1.0f };
        GLfloat spec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, pos);
        glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.03f);
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.002f);
    }
    else if (mode == 3)
    {
        glEnable(GL_LIGHT0);

        GLfloat amb[] = { 0.2f, 0.2f, 0.2f, 1.0f };
        GLfloat pos[] = { 0.0f, 12.0f, 8.0f, 1.0f };
        GLfloat dir[] = { 0.0f, -1.0f, -0.6f };
        GLfloat diff[] = { 1.0f, 1.0f, 0.85f, 1.0f };
        GLfloat spec[] = { 1.0f, 1.0f, 1.0f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, pos);
        glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, dir);
        glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 35.0f);
        glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 12.0f);
        glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
    }
    else if (mode == 4)
    {
        glEnable(GL_LIGHT0);

        Vec3 sun = getSunPosition();
        GLfloat pos[] = { sun.x, sun.y, sun.z, 1.0f };

        GLfloat diff[] = { 1.5f, 1.5f, 1.4f, 1.0f };
        GLfloat amb[] = { 0.7f, 0.7f, 0.75f, 1.0f };
        GLfloat spec[] = { 0.9f, 0.9f, 0.9f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, pos);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
        glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

        if (sun.y < 0.0f)
        {
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);

            glColor3f(0.75f, 0.80f, 1.0f);

            glPushMatrix();
            glTranslatef(-sun.x, 12.0f, -sun.z);
            glutSolidSphere(1.2f, 30, 30);
            glPopMatrix();

            glEnable(GL_LIGHTING);
            return;
        }
    }
    else
    {
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);

        GLfloat amb[] = { 0.18f, 0.18f, 0.18f, 1.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
        glLightfv(GL_LIGHT1, GL_AMBIENT, amb);

        GLfloat pos1[] = { -7.0f, 7.0f, 4.0f, 1.0f };
        GLfloat pos2[] = { 7.0f, 7.0f, 4.0f, 1.0f };

        GLfloat red[] = { 1.0f, 0.25f, 0.25f, 1.0f };
        GLfloat blue[] = { 0.25f, 0.35f, 1.0f, 1.0f };
        GLfloat spec[] = { 1.0f, 1.0f, 1.0f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, pos1);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, red);
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

        glLightfv(GL_LIGHT1, GL_POSITION, pos2);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, blue);
        glLightfv(GL_LIGHT1, GL_SPECULAR, spec);
    }
}

void drawLightning(float x, float z)
{
    if (!lightningActive) return;

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glColor3f(1.0f, 1.0f, 0.2f);
    glLineWidth(8.0f);

    glBegin(GL_LINE_STRIP);

    float yTop = 35.0f;
    float yBottom = surfaceHeight(x, z);

    float currX = x;
    float currZ = z;

    for (int i = 0; i < 10; i++)
    {
        float t = i / 10.0f;
        float y = yTop * (1.0f - t) + yBottom * t;

        currX += ((rand() % 100) / 100.0f - 0.5f) * 0.5f;
        currZ += ((rand() % 100) / 100.0f - 0.5f) * 0.5f;

        glVertex3f(currX, y, currZ);
    }

    glEnd();

    glLineWidth(1.0f);

    glEnable(GL_LIGHTING);
}

void drawTexturedCube(float sx, float sy, float sz)
{
    glPushMatrix();
    glScalef(sx, sy, sz);
    glBegin(GL_QUADS);

    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(-0.5f, -0.5f, 0.5f);
    glTexCoord2f(1, 0); glVertex3f(0.5f, -0.5f, 0.5f);
    glTexCoord2f(1, 1); glVertex3f(0.5f, 0.5f, 0.5f);
    glTexCoord2f(0, 1); glVertex3f(-0.5f, 0.5f, 0.5f);

    glNormal3f(0, 0, -1);
    glTexCoord2f(0, 0); glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 0); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 1); glVertex3f(-0.5f, 0.5f, -0.5f);
    glTexCoord2f(0, 1); glVertex3f(0.5f, 0.5f, -0.5f);

    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 0); glVertex3f(-0.5f, -0.5f, 0.5f);
    glTexCoord2f(1, 1); glVertex3f(-0.5f, 0.5f, 0.5f);
    glTexCoord2f(0, 1); glVertex3f(-0.5f, 0.5f, -0.5f);

    glNormal3f(1, 0, 0);
    glTexCoord2f(0, 0); glVertex3f(0.5f, -0.5f, 0.5f);
    glTexCoord2f(1, 0); glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 1); glVertex3f(0.5f, 0.5f, -0.5f);
    glTexCoord2f(0, 1); glVertex3f(0.5f, 0.5f, 0.5f);

    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0); glVertex3f(-0.5f, 0.5f, 0.5f);
    glTexCoord2f(1, 0); glVertex3f(0.5f, 0.5f, 0.5f);
    glTexCoord2f(1, 1); glVertex3f(0.5f, 0.5f, -0.5f);
    glTexCoord2f(0, 1); glVertex3f(-0.5f, 0.5f, -0.5f);

    glNormal3f(0, -1, 0);
    glTexCoord2f(0, 0); glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 0); glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 1); glVertex3f(0.5f, -0.5f, 0.5f);
    glTexCoord2f(0, 1); glVertex3f(-0.5f, -0.5f, 0.5f);

    glEnd();
    glPopMatrix();
}

void drawBox(float w, float h, float d)
{
    drawTexturedCube(w, h, d);
}

void drawSphere(float r)
{
    glutSolidSphere(r, 28, 28);
}

void drawCylinderX(float radius, float height)
{
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);

    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glTranslatef(0.0f, 0.0f, -height / 2.0f);
    gluCylinder(q, radius, radius, height, 32, 16);
    gluDisk(q, 0.0f, radius, 32, 1);
    glTranslatef(0.0f, 0.0f, height);
    gluDisk(q, 0.0f, radius, 32, 1);
    glPopMatrix();

    gluDeleteQuadric(q);
}

void drawCylinderY(float radius, float height)
{
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);

    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, radius, radius, height, 32, 16);
    gluDisk(q, 0.0f, radius, 32, 1);
    glTranslatef(0.0f, 0.0f, height);
    gluDisk(q, 0.0f, radius, 32, 1);
    glPopMatrix();

    gluDeleteQuadric(q);
}

void drawConeY(float radius, float height)
{
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);

    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(q, radius, 0.0f, height, 32, 16);
    gluDisk(q, 0.0f, radius, 32, 1);
    glPopMatrix();

    gluDeleteQuadric(q);
}

void drawRobot(const RobotState& rob, int index)
{
    glDisable(GL_LIGHTING);
    glColor3f(1, 0, 0);
    glPointSize(10);
    glBegin(GL_POINTS);
    glVertex3f(0, 0, 0);
    glEnd();
    glEnable(GL_LIGHTING);

    float R, G, B;
    if (index == 0) { R = 1.0f; G = 0.68f; B = 0.82f; }
    else if (index == 1) { R = 0.90f; G = 0.35f; B = 0.48f; }
    else { R = 0.95f; G = 0.50f; B = 0.68f; }

    float walkAngle = rob.walking ? 35.0f * sinf(rob.animationTime * 5.0f) : 0.0f;
    float bodyBounce = rob.walking ? 0.08f * fabsf(sinf(rob.animationTime * 5.0f)) : 0.0f;
    float danceAngle = (rob.behaviorState == 3) ? 8.0f * sinf(rob.animationTime * 8.0f) : 0.0f;

    glPushMatrix();
    float extraFallY = 0.0f;

    if (storyMode && index == 0)
    {
        extraFallY = robotAFallOffsetY;
    }

    glTranslatef(
        rob.x,
        rob.y + bodyBounce + extraFallY + ROBOT_ROOT_ABOVE_GROUND,
        rob.z
    );
    glRotatef(rob.heading + robotRotationY + danceAngle, 0.0f, 1.0f, 0.0f);
    if (storyMode && index == 0)
    {
        glRotatef(-robotAFallAngle, 1.0f, 0.0f, 0.0f);
    }
    glScalef(rob.scale, rob.scale, rob.scale);
    applyRobotMaterial(index);

    if (texturesEnabled && robotTexture != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, robotTexture);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
    }

    glColor3f(R, G, B);
    drawBox(2.0f, 2.5f, 1.25f);

    glDisable(GL_TEXTURE_2D);

    glColor3f(0.25f, 0.25f, 0.30f);
    glPushMatrix();
    glTranslatef(0, 0.35f, 0.68f);
    drawBox(1.35f, 0.50f, 0.10f);
    glPopMatrix();

    glColor3f(0.5f, 0.5f, 0.55f);
    glPushMatrix();
    glTranslatef(0, 1.45f, 0);
    drawCylinderY(0.22f, 0.45f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0, 2.30f, 0);

    glColor3f(R, G, B);
    drawBox(1.45f, 1.20f, 1.20f);

    glColor3f(0.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(-0.33f, 0.18f, 0.63f);
    drawSphere(0.16f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.33f, 0.18f, 0.63f);
    drawSphere(0.16f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.25f, 0.65f);
    drawBox(0.50f, 0.08f, 0.05f);
    glPopMatrix();

    glColor3f(0.30f, 0.30f, 0.35f);
    glPushMatrix();
    glTranslatef(0.0f, 0.72f, 0.0f);
    drawCylinderY(0.04f, 0.48f);
    glTranslatef(0.0f, 0.52f, 0.0f);
    glutSolidSphere(0.12f, 24, 24);
    glPopMatrix();

    if (index == 1)
    {
        glColor3f(0.20f, 0.20f, 0.25f);

        glPushMatrix();
        glTranslatef(-0.85f, 0.10f, 0.0f);
        drawSphere(0.12f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.85f, 0.10f, 0.0f);
        drawSphere(0.12f);
        glPopMatrix();
    }
    else if (index == 2)
    {
        glColor3f(0.85f, 0.85f, 0.90f);

        glPushMatrix();
        glTranslatef(0.0f, 0.82f, 0.0f);
        drawCylinderY(0.18f, 0.08f);
        glPopMatrix();
    }

    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.25f, 0.85f, 0);
    glRotatef(-walkAngle, 1.0f, 0.0f, 0.0f);

    glColor3f(0.40f, 0.40f, 0.45f);
    drawSphere(0.30f);

    glColor3f(R, G, B);
    glTranslatef(0, -0.75f, 0);
    drawBox(0.45f, 1.10f, 0.45f);

    glTranslatef(0, -0.65f, 0);
    glColor3f(0.40f, 0.40f, 0.45f);
    drawCylinderX(0.18f, 0.55f);

    glTranslatef(0, -0.65f, 0);
    glColor3f(R, G, B);
    drawBox(0.42f, 0.95f, 0.42f);

    glTranslatef(0, -0.55f, 0.05f);
    glColor3f(0.25f, 0.25f, 0.30f);
    drawBox(0.45f, 0.25f, 0.30f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.25f, 0.85f, 0);

    float shoulderX = walkAngle;
    float shoulderZ = 0.0f;
    float elbow = 0.0f;
    float wrist = 0.0f;

    if (rob.waving)
    {
        shoulderX = -85.0f;
        shoulderZ = 15.0f * sinf(rob.animationTime * 6.0f);
        elbow = -70.0f;
        wrist = 35.0f * sinf(rob.animationTime * 12.0f);
    }

    glRotatef(shoulderZ, 0.0f, 0.0f, 1.0f);
    glRotatef(shoulderX, 1.0f, 0.0f, 0.0f);

    glColor3f(0.40f, 0.40f, 0.45f);
    drawSphere(0.30f);

    glColor3f(R, G, B);
    glTranslatef(0, -0.75f, 0);
    drawBox(0.45f, 1.10f, 0.45f);

    glTranslatef(0, -0.65f, 0);
    glRotatef(elbow, 1.0f, 0.0f, 0.0f);
    glColor3f(0.40f, 0.40f, 0.45f);
    drawCylinderX(0.18f, 0.55f);

    glTranslatef(0, -0.65f, 0);
    glColor3f(R, G, B);
    drawBox(0.42f, 0.90f, 0.42f);

    glTranslatef(0, -0.52f, 0.05f);
    glRotatef(wrist, 0.0f, 0.0f, 1.0f);
    glRotatef(18.0f, 1.0f, 0.0f, 0.0f);
    glColor3f(0.25f, 0.25f, 0.30f);
    drawBox(0.50f, 0.25f, 0.32f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.55f, -1.35f, 0);
    glRotatef(walkAngle, 1.0f, 0.0f, 0.0f);

    glColor3f(0.40f, 0.40f, 0.45f);
    drawSphere(0.28f);

    glColor3f(R, G, B);
    glTranslatef(0, -0.75f, 0);
    drawBox(0.55f, 1.10f, 0.55f);

    glTranslatef(0, -0.70f, 0);
    glColor3f(0.40f, 0.40f, 0.45f);
    drawCylinderX(0.21f, 0.62f);

    glTranslatef(0, -0.70f, 0);
    glRotatef(-fabsf(walkAngle) * 0.35f, 1.0f, 0.0f, 0.0f);
    glColor3f(R, G, B);
    drawBox(0.50f, 1.05f, 0.50f);

    glTranslatef(0, -0.60f, 0.22f);
    glColor3f(0.18f, 0.18f, 0.22f);
    drawBox(0.70f, 0.32f, 0.95f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.55f, -1.35f, 0);
    glRotatef(-walkAngle, 1.0f, 0.0f, 0.0f);

    glColor3f(0.40f, 0.40f, 0.45f);
    drawSphere(0.28f);

    glColor3f(R, G, B);
    glTranslatef(0, -0.75f, 0);
    drawBox(0.55f, 1.10f, 0.55f);

    glTranslatef(0, -0.70f, 0);
    glColor3f(0.40f, 0.40f, 0.45f);
    drawCylinderX(0.21f, 0.62f);

    glTranslatef(0, -0.70f, 0);
    glRotatef(-fabsf(walkAngle) * 0.35f, 1.0f, 0.0f, 0.0f);
    glColor3f(R, G, B);
    drawBox(0.50f, 1.05f, 0.50f);

    glTranslatef(0, -0.60f, 0.22f);
    glColor3f(0.18f, 0.18f, 0.22f);
    drawBox(0.70f, 0.32f, 0.95f);
    glPopMatrix();

    if (rob.behaviorState == 1)
    {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.95f, 0.1f);
        glPushMatrix();
        glTranslatef(0.0f, 3.5f, 0.0f);
        glutSolidSphere(0.12f + 0.04f * sinf(rob.animationTime * 10.0f), 16, 16);
        glPopMatrix();
        glEnable(GL_LIGHTING);
    }

    glPopMatrix();

    if (texturesEnabled) glEnable(GL_TEXTURE_2D);
}

void drawGround()
{
    glDisable(GL_TEXTURE_2D);

    setMatteMaterial();

    int size = 150;
    float step = 0.5f;

    for (int i = -size; i < size; i++)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = -size; j <= size; j++)
        {
            float x1 = i * step;
            float z1 = j * step;
            float y1 = surfaceHeight(x1, z1) - 1.5f;
            Vec3 n1 = surfaceNormal(x1, z1);

            glNormal3f(n1.x, n1.y, n1.z);

            if (y1 < waterLevel + 0.6f)
                glColor3f(0.85f, 0.75f, 0.45f);
            else if (y1 < 2.0f)
                glColor3f(0.55f, 0.35f, 0.20f);
            else
                glColor3f(0.35f, 0.85f, 0.45f);

            glVertex3f(x1, y1, z1);

            float x2 = (i + 1) * step;
            float z2 = j * step;
            float y2 = surfaceHeight(x2, z2) - 1.5f;
            Vec3 n2 = surfaceNormal(x2, z2);

            glNormal3f(n2.x, n2.y, n2.z);

            if (y2 < waterLevel + 0.6f)
                glColor3f(0.85f, 0.75f, 0.45f);
            else if (y2 < 2.0f)
                glColor3f(0.70f, 0.45f, 0.25f);
            else
                glColor3f(0.20f, 0.65f, 0.25f);

            glVertex3f(x2, y2, z2);
        }
        glEnd();
    }
}

void drawSky()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDepthMask(GL_FALSE);

    if (texturesEnabled && skyTexture != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, skyTexture);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
    }

    float t = timeOfDay;

    float dayFactor = sinf(t * PI);

    glColor3f(
        0.25f + 0.65f * dayFactor,
        0.30f + 0.65f * dayFactor,
        0.40f + 0.70f * dayFactor
    );

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-100.0f, -40.0f, -80.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(100.0f, -40.0f, -80.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(100.0f, 80.0f, -80.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-100.0f, 80.0f, -80.0f);
    glEnd();

    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
}

void drawFloorGrid()
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glColor3f(0.35f, 0.35f, 0.35f);
    glLineWidth(1.0f);

    int size = 100;
    float step = 1.0f;

    float offset = -1.45f;

    for (int i = -size; i <= size; i++)
    {
        glBegin(GL_LINE_STRIP);
        for (int j = -size; j <= size; j++)
        {
            float x = i * step;
            float z = j * step;
            float y = surfaceHeight(x, z) + offset;

            glVertex3f(x, y, z);
        }
        glEnd();
    }

    for (int j = -size; j <= size; j++)
    {
        glBegin(GL_LINE_STRIP);
        for (int i = -size; i <= size; i++)
        {
            float x = i * step;
            float z = j * step;
            float y = surfaceHeight(x, z) + offset;

            glVertex3f(x, y, z);
        }
        glEnd();
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    if (texturesEnabled)
        glEnable(GL_TEXTURE_2D);
}

void drawInteractionZones()
{
    if (robots.size() < 3) return;

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);

    for (int i = 0; i < 3; i++)
    {
        if (!robots[i].waving && robots[i].behaviorState == 0) continue;

        if (robots[i].behaviorState == 1) glColor3f(1.0f, 1.0f, 0.0f);
        else if (robots[i].behaviorState == 2) glColor3f(1.0f, 0.35f, 0.1f);
        else glColor3f(0.5f, 1.0f, 1.0f);

        glBegin(GL_LINE_LOOP);
        for (int k = 0; k < 64; k++)
        {
            float a = 2.0f * PI * k / 64.0f;
            float radius = 1.3f + 0.15f * sinf(robots[i].animationTime * 5.0f);
            float x = robots[i].x + radius * cosf(a);
            float z = robots[i].z + radius * sinf(a);
            glVertex3f(x, surfaceHeight(x, z) - 3.35f, z);
        }
        glEnd();
    }

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
    if (texturesEnabled) glEnable(GL_TEXTURE_2D);
}

void drawInteractionObject()
{
    if (robots.size() < 3) return;

    float d12 = sqrtf((robots[0].x - robots[1].x) * (robots[0].x - robots[1].x) + (robots[0].z - robots[1].z) * (robots[0].z - robots[1].z));
    float d23 = sqrtf((robots[1].x - robots[2].x) * (robots[1].x - robots[2].x) + (robots[1].z - robots[2].z) * (robots[1].z - robots[2].z));

    if (d12 < 3.2f || d23 < 3.0f)
    {
        RobotState& a = d12 < 3.2f ? robots[0] : robots[1];
        RobotState& b = d12 < 3.2f ? robots[1] : robots[2];

        float x = (a.x + b.x) * 0.5f;
        float z = (a.z + b.z) * 0.5f;
        float y = surfaceHeight(x, z) + 2.3f;

        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 0.85f, 0.15f);
        setMetalMaterial();

        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(lightAngle * 3.0f, 0.0f, 1.0f, 0.0f);
        drawSphere(0.25f);
        glPopMatrix();
    }
}

void drawRescueBeacon()
{
    float x = 0.0f;
    float z = 0.0f;
    float y = surfaceHeight(x, z) - 3.15f;

    glDisable(GL_TEXTURE_2D);
    setMetalMaterial();

    glPushMatrix();
    glTranslatef(x, y, z);

    glColor3f(0.25f, 0.25f, 0.30f);
    drawCylinderY(0.18f, 1.2f);

    glTranslatef(0.0f, 1.35f, 0.0f);

    if (rescueMissionComplete)
        glColor3f(0.1f, 1.0f, 0.35f);
    else
        glColor3f(1.0f, 0.15f, 0.05f);

    glutSolidSphere(0.35f + 0.07f * sinf(lightAngle * PI / 45.0f), 28, 28);
    glPopMatrix();
}

void drawGoalPosts()
{
    glDisable(GL_TEXTURE_2D);
    setMetalMaterial();
    glColor3f(0.95f, 0.95f, 1.0f);

    float goals[2] = { -14.0f, 14.0f };
    for (int i = 0; i < 2; i++)
    {
        float x = goals[i];
        float z = 0.0f;
        float y = surfaceHeight(x, z) - 3.45f;

        glPushMatrix();
        glTranslatef(x, y, z);
        drawCylinderY(0.08f, 2.0f);
        glTranslatef(0.0f, 2.0f, 0.0f);
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        drawCylinderY(0.08f, 2.2f);
        glPopMatrix();
    }
}

void spawnParticles(float x, float y, float z)
{
    if ((int)particles.size() > 250) return;

    for (int i = 0; i < 8; i++)
    {
        float a = ((float)(rand() % 360)) * PI / 180.0f;
        float s = 0.02f + (float)(rand() % 20) / 600.0f;
        Particle p;
        p.x = x;
        p.y = y;
        p.z = z;
        p.vx = cosf(a) * s;
        p.vy = 0.05f + (float)(rand() % 20) / 500.0f;
        p.vz = sinf(a) * s;
        p.life = 1.0f;
        p.maxLife = 1.0f;
        particles.push_back(p);
    }
}

void updateParticles()
{
    for (int i = 0; i < (int)particles.size(); )
    {
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].z += particles[i].vz;
        particles[i].vy -= 0.002f;
        particles[i].life -= 0.015f;

        if (particles[i].life <= 0.0f)
            particles.erase(particles.begin() + i);
        else
            i++;
    }
}

void drawParticles()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPointSize(4.0f);

    glBegin(GL_POINTS);
    for (int i = 0; i < (int)particles.size(); i++)
    {
        float a = particles[i].life / particles[i].maxLife;
        glColor4f(1.0f, 0.75f, 0.15f, a);
        glVertex3f(particles[i].x, particles[i].y, particles[i].z);
    }
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    if (texturesEnabled) glEnable(GL_TEXTURE_2D);
}

void drawShadows()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < (int)robots.size(); i++)
    {
        RobotState& r = robots[i];
        float y = surfaceHeight(r.x, r.z) - 3.48f;

        glPushMatrix();
        glTranslatef(r.x, y, r.z);
        glRotatef(r.heading, 0.0f, 1.0f, 0.0f);
        glScalef(1.2f * r.scale, 0.0f, 0.8f * r.scale);

        glColor4f(0.0f, 0.0f, 0.0f, 0.32f);

        glBegin(GL_QUADS);
        glVertex3f(-1.0f, 0.0f, -1.0f);
        glVertex3f(1.0f, 0.0f, -1.0f);
        glVertex3f(1.0f, 0.0f, 1.0f);
        glVertex3f(-1.0f, 0.0f, 1.0f);
        glEnd();

        glPopMatrix();
    }

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    if (texturesEnabled) glEnable(GL_TEXTURE_2D);
}

void applyProjection(int w, int h)
{
    if (h == 0) h = 1;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (usePerspective)
        gluPerspective(60.0, (double)w / (double)h, 0.1, 140.0);
    else
        glOrtho(-18.0, 18.0, -12.0, 18.0, -60.0, 80.0);

    glMatrixMode(GL_MODELVIEW);
}

void setCameraView()
{
    if (followCamera && robots.size() > 0)
    {
        RobotState& r = robots[followedRobot];

        float rad = r.heading * PI / 180.0f;
        float camDist = 5.5f;
        float lookDist = 7.0f;

        float targetX = r.x - sinf(rad) * camDist;
        float targetY = r.y + ROBOT_ROOT_ABOVE_GROUND + 2.3f;
        float targetZ = r.z - cosf(rad) * camDist;

        camX += (targetX - camX) * 0.10f;
        camY += (targetY - camY) * 0.10f;
        camZ += (targetZ - camZ) * 0.10f;

        camY += sinf(r.animationTime * 10.0f) * 0.06f;

        float lookX = r.x + sinf(rad) * lookDist;
        float lookY = r.y + ROBOT_ROOT_ABOVE_GROUND + 1.4f;
        float lookZ = r.z + cosf(rad) * lookDist;

        gluLookAt(
            camX, camY, camZ,
            lookX, lookY, lookZ,
            0.0f, 1.0f, 0.0f
        );

        return;
    }

    if (currentView == 1)
    {
        gluLookAt(
            0.12f, 15.92f, 35.60f,
            0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f
        );
    }
    else if (currentView == 2)
    {
        gluLookAt(24.0, 6.0, 0.0,
            0.0, 2.0, 0.0,
            0.0, 1.0, 0.0);
    }
    else if (currentView == 3)
    {
        gluLookAt(0.0, 30.0, 0.1,
            0.0, 0.0, 0.0,
            0.0, 0.0, -1.0);
    }
    else
    {
        float yaw = orbitYaw * PI / 180.0f;
        float pitch = orbitPitch * PI / 180.0f;

        float x = orbitDistance * cosf(pitch) * sinf(yaw);
        float y = orbitDistance * sinf(pitch);
        float z = orbitDistance * cosf(pitch) * cosf(yaw);

        gluLookAt(
            x, y, z,
            0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f
        );
    }
}

void drawScene()
{
    drawSky();

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    drawGround();

    if (showGrid)
    {
        drawFloorGrid();
        drawInteractionZones();
    }

    for (int i = 0; i < (int)robots.size(); i++)
    {
        drawRobot(robots[i], i);
    }

    drawLightning(robots[0].x, robots[0].z);

    drawRescueBeacon();
    drawGoalPosts();
    drawParticles();
    drawShadows();

    if (specialDanceMode)
    {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);

        glColor3f(1.0f, 0.2f, 0.7f);

        glPushMatrix();
        glTranslatef(ballPos.x, ballPos.y, ballPos.z);
        glPopMatrix();

        glPointSize(3.0f);
        glBegin(GL_POINTS);

        for (int i = 0; i < 20; i++)
        {
            float a = ((rand() % 360) * PI) / 180.0f;
            float r = ((rand() % 100) / 100.0f) * 0.8f;

            float x = ballPos.x + cosf(a) * r;
            float y = ballPos.y + ((rand() % 100) / 100.0f) * 0.8f;
            float z = ballPos.z + sinf(a) * r;

            glColor3f(1.0f, 0.6f, 0.9f);
            glVertex3f(x, y, z);
        }

        glEnd();

        glEnable(GL_LIGHTING);
    }

    glEnable(GL_LIGHTING);
    if (texturesEnabled) glEnable(GL_TEXTURE_2D);
}

void renderViewport(int x, int y, int w, int h, int view)
{
    glViewport(x, y, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (usePerspective)
        gluPerspective(60.0, (float)w / (float)h, 0.1, 140.0);
    else
        glOrtho(-18.0, 18.0, -12.0, 18.0, -60.0, 80.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    int oldView = currentView;
    currentView = view;

    setCameraView();
    setupLighting(lightingMode);
    drawScene();

    currentView = oldView;
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    if (splitScreen)
    {
        int halfW = w / 2;
        int halfH = h / 2;

        renderViewport(0, halfH, halfW, halfH, 1);
        renderViewport(halfW, halfH, halfW, halfH, 2);
        renderViewport(0, 0, halfW, halfH, 3);
        renderViewport(halfW, 0, halfW, halfH, 4);
    }
    else
    {
        renderViewport(0, 0, w, h, currentView);
    }

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    applyProjection(w, h);
}

void resetRobots()
{
    robots.clear();

    robots.push_back({ startA.x, 0, startA.z, 0,0,0,0,false,false,0,1,0,0 });
    robots.push_back({ startB.x, 0, startB.z, 0,0,0,0,false,false,0,1,0,0 });
    robots.push_back({ startC.x, 0, startC.z, 0,0,180,0,false,false,0,1,0,0 });

    storyState = STORY_START_POS;
    storyTimer = 0.0f;

    camX = 0.0f;
    camY = 6.0f;
    camZ = 16.0f;

    robotAFallAngle = 0.0f;
    robotAFallOffsetY = 0.0f;

    ballOwner = 0;
    ballMoving = false;
    ballT = 0.0f;

    sparksSpawned = false;
}

float distanceXZ(const RobotState& a, const RobotState& b)
{
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dz * dz);
}

void handleRobotInteractions()
{
    if (robots.size() < 3) return;

    for (auto& r : robots)
    {
        r.waving = false;
        r.behaviorState = 0;
        r.pathOffset = 0.0f;
    }

    robots[0].speed = 0.0038f;
    robots[1].speed = 0.0032f;
    robots[2].speed = 0.0035f;

    robots[0].walking = true;
    robots[1].walking = true;
    robots[2].walking = true;

    rescueMissionComplete = false;

    if (!interactionsEnabled) return;

    float d12 = distanceXZ(robots[0], robots[1]);
    float d23 = distanceXZ(robots[1], robots[2]);
    float d13 = distanceXZ(robots[0], robots[2]);

    if (d12 < 3.2f && robots[0].interactionCooldown <= 0.0f)
    {
        robots[0].waving = true;
        robots[1].waving = true;
        robots[1].walking = false;

        robots[0].behaviorState = 1;
        robots[1].behaviorState = 1;

        robots[0].interactionCooldown = 1.0f;
        robots[1].interactionCooldown = 1.0f;

        spawnParticles((robots[0].x + robots[1].x) * 0.5f,
            (robots[0].y + robots[1].y) * 0.5f + 5.5f,
            (robots[0].z + robots[1].z) * 0.5f);
    }

    if (d23 < 3.0f && robots[2].interactionCooldown <= 0.0f)
    {
        robots[2].speed = 0.0015f;
        robots[1].speed = 0.0020f;

        robots[2].waving = true;
        robots[1].waving = true;

        robots[2].behaviorState = 2;
        robots[1].behaviorState = 2;

        robots[2].pathOffset = 0.45f * sinf(robots[2].animationTime * 4.0f);

        robots[2].interactionCooldown = 1.0f;
        robots[1].interactionCooldown = 1.0f;

        spawnParticles((robots[1].x + robots[2].x) * 0.5f,
            (robots[1].y + robots[2].y) * 0.5f + 5.0f,
            (robots[1].z + robots[2].z) * 0.5f);
    }

    if (d13 < 3.5f && robots[0].interactionCooldown <= 0.0f)
    {
        robots[0].waving = true;
        robots[2].waving = true;

        robots[0].speed = 0.0020f;
        robots[2].speed = 0.0020f;

        robots[0].behaviorState = 3;
        robots[2].behaviorState = 3;

        robots[0].interactionCooldown = 1.0f;
        robots[2].interactionCooldown = 1.0f;
    }

    float rescueRadius = 3.0f;

    float d1Beacon = sqrtf(robots[0].x * robots[0].x + robots[0].z * robots[0].z);
    float d2Beacon = sqrtf(robots[1].x * robots[1].x + robots[1].z * robots[1].z);
    float d3Beacon = sqrtf(robots[2].x * robots[2].x + robots[2].z * robots[2].z);

    if (d1Beacon < rescueRadius &&
        d2Beacon < rescueRadius &&
        d3Beacon < rescueRadius)
    {
        rescueMissionComplete = true;

        robots[0].waving = true;
        robots[1].waving = true;
        robots[2].waving = true;

        robots[0].behaviorState = 3;
        robots[1].behaviorState = 3;
        robots[2].behaviorState = 3;

        spawnParticles(
            0.0f,
            surfaceHeight(0.0f, 0.0f) + 1.0f,
            0.0f
        );
    }
}

void updateRobots()
{
    if (specialDanceMode)
    {
        for (int i = 0; i < robots.size(); i++)
        {
            RobotState& r = robots[i];

            float t = danceTimer;

            float angle = t * 2.0f + i * 2.0f;
            float radius = 5.0f;

            r.x = cosf(angle) * radius;
            r.z = sinf(angle) * radius;

            r.y = getGroundY(r.x, r.z) + fabsf(sinf(t * 7.0f)) * 4.0f;

            r.heading = angle * 180.0f / PI;

            r.walking = true;
            r.waving = false;

            r.animationTime += animationSpeed;
        }

        if (!ballMoving)
        {
            RobotState& r = robots[ballOwner];

            ballPos.x = r.x;
            ballPos.y = r.y + 2.5f;
            ballPos.z = r.z;

            if (danceTimer > 0.8f)
            {
                ballMoving = true;
                ballT = 0.0f;
                danceTimer = 0.0f;
            }
        }
        else
        {
            int next = (ballOwner + 1) % robots.size();

            RobotState& A = robots[ballOwner];
            RobotState& B = robots[next];

            ballT += 0.05f;

            float t = ballT;

            ballPos.x = A.x * (1 - t) + B.x * t;
            ballPos.z = A.z * (1 - t) + B.z * t;

            ballPos.y = getGroundY(ballPos.x, ballPos.z)
                + 2.0f
                + sinf(t * PI) * (2.5f + 0.5f * sinf(danceTimer * 6.0f));

            if (ballT >= 1.0f)
            {
                ballOwner = next;
                ballMoving = false;
                ballT = 0.0f;
            }
        }

        danceTimer += animationSpeed;

        return;
    }

    if (randomWalkMode)
    {
        for (int i = 0; i < robots.size(); i++)
        {
            RobotState& r = robots[i];

            if ((rand() % 100) < 2)
            {
                r.heading = (float)(rand() % 360);
            }

            float rad = r.heading * PI / 180.0f;

            float speed = 0.08f + (i * 0.02f);

            r.x += sinf(rad) * speed;
            r.z += cosf(rad) * speed;

            r.y = getGroundY(r.x, r.z);

            r.walking = true;
            r.waving = false;

            r.animationTime += animationSpeed;
        }

        return;
    }

    if (storyMode)
    {
        RobotState& A = robots[0];
        RobotState& B = robots[1];
        RobotState& C = robots[2];

        storyTimer += animationSpeed;

        A.animationTime += animationSpeed;
        B.animationTime += animationSpeed;
        C.animationTime += animationSpeed;

        A.walking = B.walking = C.walking = false;

        if (storyState == STORY_START_POS)
        {
            if (storyTimer > 1.0f)
            {
                storyState = STORY_MOVE_TO_CENTER;
                storyTimer = 0.0f;
            }
        }

        else if (storyState == STORY_MOVE_TO_CENTER)
        {
            A.waving = false;
            B.waving = false;
            C.waving = false;

            A.walking = B.walking = C.walking = true;

            Vec3 targetA = semiA;
            Vec3 targetB = semiB;
            Vec3 targetC = semiC;

            float totalDistA = sqrtf((startA.x - targetA.x) * (startA.x - targetA.x) + (startA.z - targetA.z) * (startA.z - targetA.z));
            float totalDistB = sqrtf((startB.x - targetB.x) * (startB.x - targetB.x) + (startB.z - targetB.z) * (startB.z - targetB.z));
            float totalDistC = sqrtf((startC.x - targetC.x) * (startC.x - targetC.x) + (startC.z - targetC.z) * (startC.z - targetC.z));

            float currDistA = sqrtf((A.x - targetA.x) * (A.x - targetA.x) + (A.z - targetA.z) * (A.z - targetA.z));
            float currDistB = sqrtf((B.x - targetB.x) * (B.x - targetB.x) + (B.z - targetB.z) * (B.z - targetB.z));
            float currDistC = sqrtf((C.x - targetC.x) * (C.x - targetC.x) + (C.z - targetC.z) * (C.z - targetC.z));

            float progressA = 1.0f - currDistA / totalDistA;
            float progressB = 1.0f - currDistB / totalDistB;
            float progressC = 1.0f - currDistC / totalDistC;

            A.waving = (A.walking && progressA > 0.35f && progressA < 0.95f);
            B.waving = (B.walking && progressB > 0.35f && progressB < 0.95f);
            C.waving = (C.walking && progressC > 0.35f && progressC < 0.95f);

            float threshold = 0.3f;

            Vec3 diffA = sub(targetA, makeVec3(A.x, 0, A.z));
            float distA = sqrtf(diffA.x * diffA.x + diffA.z * diffA.z);

            if (distA > threshold)
            {
                Vec3 newA = moveTowards(makeVec3(A.x, 0, A.z), targetA, 0.12f);
                A.x = newA.x;
                A.z = newA.z;
            }
            else
            {
                A.x = targetA.x;
                A.z = targetA.z;
            }

            Vec3 diffB = sub(targetB, makeVec3(B.x, 0, B.z));
            float distB = sqrtf(diffB.x * diffB.x + diffB.z * diffB.z);

            if (distB > threshold)
            {
                Vec3 newB = moveTowards(makeVec3(B.x, 0, B.z), targetB, 0.12f);
                B.x = newB.x;
                B.z = newB.z;
            }
            else
            {
                B.x = targetB.x;
                B.z = targetB.z;
            }

            Vec3 diffC = sub(targetC, makeVec3(C.x, 0, C.z));
            float distC = sqrtf(diffC.x * diffC.x + diffC.z * diffC.z);

            if (distC > threshold)
            {
                Vec3 newC = moveTowards(makeVec3(C.x, 0, C.z), targetC, 0.12f);
                C.x = newC.x;
                C.z = newC.z;
            }
            else
            {
                C.x = targetC.x;
                C.z = targetC.z;
            }

            Vec3 dirA = sub(targetA, makeVec3(A.x, 0, A.z));
            Vec3 dirB = sub(targetB, makeVec3(B.x, 0, B.z));
            Vec3 dirC = sub(targetC, makeVec3(C.x, 0, C.z));

            A.heading = atan2f(dirA.x, dirA.z) * 180.0f / PI;
            B.heading = atan2f(dirB.x, dirB.z) * 180.0f / PI;
            C.heading = atan2f(dirC.x, dirC.z) * 180.0f / PI;

            if (fabs(A.x - targetA.x) < 0.01f &&
                fabs(B.x - targetB.x) < 0.01f &&
                fabs(C.x - targetC.x) < 0.01f)
            {
                A.walking = false;
                B.walking = false;
                C.walking = false;

                A.heading = 0.0f;
                B.heading = 0.0f;
                C.heading = 0.0f;

                storyState = STORY_LIE_DOWN;
                storyTimer = 0.0f;
            }
        }

        else if (storyState == STORY_LIE_DOWN)
        {
            A.waving = false;
            B.waving = false;
            C.waving = false;

            A.heading = 0.0f;
            B.heading = 0.0f;
            C.heading = 0.0f;

            A.scale = 1.0f;
            B.scale = 1.0f;
            C.scale = 1.0f;

            if (storyTimer > 2.5f)
            {
                storyState = STORY_LIGHTNING_STRIKE;
                storyTimer = 0.0f;
                lightningTimer = 0.0f;
                lightningActive = true;
            }
        }

        else if (storyState == STORY_LIGHTNING_STRIKE)
        {
            A.walking = false;
            B.walking = false;
            C.walking = false;

            lightningTimer += animationSpeed;

            if (lightningTimer > 0.5f)
            {
                lightningActive = false;
                storyState = STORY_SPARKS;
                storyTimer = 0.0f;
            }
        }

        else if (storyState == STORY_SPARKS)
        {
            A.waving = false;
            B.waving = false;
            C.waving = false;

            if (robotAFallAngle < 90.0f)
            {
                robotAFallAngle += 2.0f;
                robotAFallOffsetY -= 0.02f;
            }

            spawnParticles(A.x, A.y + 2.5f, A.z);
            spawnParticles(A.x, A.y + 2.5f, A.z);
            spawnParticles(A.x, A.y + 2.5f, A.z);

            if (storyTimer > 2.0f)
            {
                storyState = STORY_REACT_TO_FALL;
                storyTimer = 0.0f;
            }
        }

        else if (storyState == STORY_REACT_TO_FALL)
        {
            A.waving = false;
            B.waving = false;
            C.waving = false;

            B.walking = false;
            C.walking = false;

            Vec3 dirB = sub(makeVec3(A.x, 0, A.z), makeVec3(B.x, 0, B.z));
            Vec3 dirC = sub(makeVec3(A.x, 0, A.z), makeVec3(C.x, 0, C.z));

            B.heading = atan2f(dirB.x, dirB.z) * 180.0f / PI;
            C.heading = atan2f(dirC.x, dirC.z) * 180.0f / PI;

            spawnParticles(A.x, A.y + 2.5f, A.z);

            if (storyTimer > 2.5f)
            {
                storyState = STORY_RUN_AWAY;
                storyTimer = 0.0f;
            }
        }

        else if (storyState == STORY_RUN_AWAY)
        {
      
            A.waving = false;
            B.waving = false;
            C.waving = false;

            B.walking = true;
            C.walking = true;

            Vec3 runDir = makeVec3(1.0f, 0.0f, -1.0f);

            float len = sqrtf(runDir.x * runDir.x + runDir.z * runDir.z);
            runDir.x /= len;
            runDir.z /= len;

            float t = storyTimer;

            float speed = 0.15f + t * 0.6f;

            if (speed > 0.8f) speed = 0.8f;

            B.x += runDir.x * speed;
            B.z += runDir.z * speed;

            C.x += runDir.x * speed;
            C.z += runDir.z * speed;

            B.heading = atan2f(runDir.x, runDir.z) * 180.0f / PI;
            C.heading = atan2f(runDir.x, runDir.z) * 180.0f / PI;
        }

        A.y = getGroundY(A.x, A.z);
        B.y = getGroundY(B.x, B.z);
        C.y = getGroundY(C.x, C.z);

        return;
    }

    handleRobotInteractions();

    for (int i = 0; i < (int)robots.size(); i++)
    {
        RobotState& r = robots[i];

        if (r.interactionCooldown > 0.0f)
            r.interactionCooldown -= 0.02f;

        if (r.walking)
        {
            r.pathT += r.speed * (animationSpeed / 0.05f);
            if (r.pathT > 1.0f) r.pathT -= 1.0f;
        }

        Vec3 p = evaluatePath(i, r.pathT);
        Vec3 tangent = evaluatePathTangent(i, r.pathT);
        Vec3 side = normalizeVec(makeVec3(tangent.z, 0.0f, -tangent.x));

        p.x += side.x * r.pathOffset;
        p.z += side.z * r.pathOffset;
        p.y = surfaceHeight(p.x, p.z);

        r.x = p.x;
        r.y = getGroundY(r.x, r.z);
        r.z = p.z;

        r.heading = atan2f(tangent.x, tangent.z) * 180.0f / PI;
        r.animationTime += animationSpeed;
    }
}

void timer(int value)
{
    if (!isPaused)
    {
        updateRobots();
        updateParticles();

        timeOfDay += 0.0025f;

        if (timeOfDay > 1.0f)
        {
            timeOfDay = 0.0f;
        }

        lightAngle += 1.5f;
        if (lightAngle > 360.0f) lightAngle -= 360.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int, int)
{
    switch (key)
    {
    case '1': currentView = 1; followCamera = false; break;
    case '2': currentView = 2; followCamera = false; break;
    case '3': currentView = 3; followCamera = false; break;
    case '4': currentView = 4; followCamera = false; break;

    case 'p':
    case 'P': usePerspective = true; break;

    case 'o':
    case 'O': usePerspective = false; break;

    case 'w':
    case 'W':

        randomWalkMode = !randomWalkMode;

        if (randomWalkMode)
        {
            savedRobots = robots;

            savedStoryState = storyState;
            savedStoryTimer = storyTimer;
            savedFallAngle = robotAFallAngle;
            savedFallOffsetY = robotAFallOffsetY;
            savedLightning = lightningActive;
            savedLightningTimer = lightningTimer;

            storyMode = false;
            specialDanceMode = false;

            for (auto& r : robots)
                r.walking = true;
        }
        else
        {
            robots = savedRobots;

            storyState = savedStoryState;
            storyTimer = savedStoryTimer;
            robotAFallAngle = savedFallAngle;
            robotAFallOffsetY = savedFallOffsetY;
            lightningActive = savedLightning;
            lightningTimer = savedLightningTimer;

            storyMode = true;
        }

        break;

    case 'i':
    case 'I':

        if (!specialDanceMode)
        {
            savedRobots = robots;
        }

        specialDanceMode = !specialDanceMode;
        isPaused = false;

        if (!specialDanceMode)
        {
            robots = savedRobots;
            storyMode = true;
        }
        else
        {
            storyMode = false;
            danceTimer = 0.0f;
        }

        break;

    case 'l':
    case 'L':
        lightingMode++;
        if (lightingMode > 5) lightingMode = 1;
        printf("Lighting mode: %d\n", lightingMode);
        break;

    case 't':
    case 'T':
        texturesEnabled = !texturesEnabled;
        break;

    case 'g':
    case 'G':
        showGrid = !showGrid;
        break;

    case 'm':
    case 'M':
        splitScreen = !splitScreen;
        break;

    case 'b':
    case 'B':
        showLabels = false;
        break;

    case '+':
        animationSpeed += 0.05f;
        if (animationSpeed > 0.5f) animationSpeed = 0.5f;
        printf("Speed: %f\n", animationSpeed);
        break;

    case '-':
        animationSpeed -= 0.05f;
        if (animationSpeed < 0.01f) animationSpeed = 0.01f;
        printf("Speed: %f\n", animationSpeed);
        break;

    case 'a':
    case 'A':
        robotRotationY -= 5.0f;
        break;

    case 'd':
    case 'D':
        robotRotationY += 5.0f;
        break;

    case 'r':
    case 'R':

        currentView = 1;
        usePerspective = true;
        interactionsEnabled = true;
        followCamera = false;
        splitScreen = false;

        lightingMode = 3;
        animationSpeed = 0.05f;
        robotRotationY = 0.0f;

        timeOfDay = 0.0f;

        orbitYaw = 45.0f;
        orbitPitch = 28.0f;
        orbitDistance = 25.0f;

        resetRobots();

        lightningActive = false;
        lightningTimer = 0.0f;
        particles.clear();
        storyMode = true;
        storyState = STORY_START_POS;
        storyTimer = 0.0f;
        robotAFallAngle = 0.0f;
        robotAFallOffsetY = 0.0f;

        sparksSpawned = false;
        rescueMissionComplete = false;

        break;

    case 'f':
    case 'F':
        followCamera = !followCamera;
        break;

    case 'z': followedRobot = 0; followCamera = true; break;
    case 'x': followedRobot = 1; followCamera = true; break;
    case 'c': followedRobot = 2; followCamera = true; break;

    case ' ':
        isPaused = !isPaused;
        break;

    case 'k':
        storyMode = !storyMode;
        break;

    case 27:
        std::exit(0);
        break;
    }

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    applyProjection(w, h);
    glutPostRedisplay();
}

void specialKeyboard(int key, int, int)
{
    if (key == GLUT_KEY_LEFT) orbitYaw -= 3.0f;
    if (key == GLUT_KEY_RIGHT) orbitYaw += 3.0f;
    if (key == GLUT_KEY_UP) orbitPitch += 2.0f;
    if (key == GLUT_KEY_DOWN) orbitPitch -= 2.0f;

    if (orbitPitch > 80.0f) orbitPitch = 80.0f;
    if (orbitPitch < -5.0f) orbitPitch = -5.0f;

    glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        mouseDragging = (state == GLUT_DOWN);
        lastMouseX = x;
        lastMouseY = y;
    }

    if (button == 3)
    {
        orbitDistance -= 1.0f;
        if (orbitDistance < 10.0f) orbitDistance = 10.0f;
    }

    if (button == 4)
    {
        orbitDistance += 1.0f;
        if (orbitDistance > 55.0f) orbitDistance = 55.0f;
    }

    glutPostRedisplay();
}

void mouseMotion(int x, int y)
{
    if (!mouseDragging) return;

    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    orbitYaw += dx * 0.4f;
    orbitPitch += dy * 0.3f;

    if (orbitPitch > 80.0f) orbitPitch = 80.0f;
    if (orbitPitch < -5.0f) orbitPitch = -5.0f;

    lastMouseX = x;
    lastMouseY = y;

    currentView = 4;
    followCamera = false;
    glutPostRedisplay();
}

void init()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.92f, 0.95f, 1.0f, 1.0f);

    robotTexture = loadTexture("C:\\Users\\Deborah\\Documents\\GUI\\test\\x64\\Debug\\robot_texture.bmp");
    floorTexture = loadTexture("C:\\Users\\Deborah\\Documents\\GUI\\test\\x64\\Debug\\floor_texture.bmp");
    skyTexture = loadTexture("C:\\Users\\Deborah\\Documents\\GUI\\test\\x64\\Debug\\sky_texture.bmp");

    if (robotTexture == 0)
        robotTexture = createCheckerTexture(64, 255, 170, 210, 235, 120, 170);

    if (floorTexture == 0)
        floorTexture = createCheckerTexture(64, 120, 155, 125, 90, 125, 95);

    if (skyTexture == 0)
        skyTexture = createGradientTexture(64, 64);

    pathTexture = createCheckerTexture(32, 255, 255, 255, 180, 180, 180);

    printf("Textures loaded: %u %u %u\n", robotTexture, floorTexture, skyTexture);
    printf("Controls: 1/2/3/4 views, P/O projection, W walking, I interactions, L lighting, T textures, G guides, M split-screen, F follow camera, Z/X/C follow robot, +/- speed, mouse orbit, R reset, ESC exit\n");

    resetRobots();

    timeOfDay = 0.0f;

    glEnable(GL_FOG);

    GLfloat fogColor[4] = { 0.92f, 0.95f, 1.0f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);

    glFogf(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 80.0f);
    glFogf(GL_FOG_END, 200.0f);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000, 700);
    glutCreateWindow("Robot Interaction Arena - OpenGL Final Project");

    init();

    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    applyProjection(w, h);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
