#include <GL/freeglut.h>
#include <GL/glu.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

const float PI = 3.1415926535f;

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
    float anim;
    float scale;
};

std::vector<RobotState> robots;

GLuint floorTexture = 0;
GLuint backdropTexture = 0;
GLuint robotTexture = 0;

int currentView = 4;
int lightingMode = 3;
int storyStep = 0;
int followedRobot = 0;

bool usePerspective = true;
bool texturesEnabled = true;
bool showGuides = true;
bool splitScreen = false;
bool storyMode = false;
bool freeExplore = true;

float orbitYaw = 45.0f;
float orbitPitch = 24.0f;
float orbitDistance = 30.0f;
float animationSpeed = 0.05f;
float storyTime = 0.0f;
float sunsetAmount = 0.0f;
float lightAngle = 0.0f;

bool mouseDragging = false;
int lastMouseX = 0;
int lastMouseY = 0;

Vec3 makeVec3(float x, float y, float z)
{
    Vec3 v = { x, y, z };
    return v;
}

Vec3 sub(Vec3 a, Vec3 b)
{
    return makeVec3(a.x - b.x, a.y - b.y, a.z - b.z);
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

float surfaceHeight(float x, float z)
{
    return 0.35f * sinf(0.25f * x) +
        0.25f * cosf(0.22f * z) +
        0.18f * sinf(0.14f * x + 0.18f * z);
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
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float u = 1.0f - t;

    float b0 = u * u * u;
    float b1 = 3.0f * u * u * t;
    float b2 = 3.0f * u * t * t;
    float b3 = t * t * t;

    Vec3 p;
    p.x = b0 * p0.x + b1 * p1.x + b2 * p2.x + b3 * p3.x;
    p.z = b0 * p0.z + b1 * p1.z + b2 * p2.z + b3 * p3.z;
    p.y = surfaceHeight(p.x, p.z);

    return p;
}

Vec3 evaluatePath(int id, float t)
{
    if (id == 0)
    {
        return bezierPoint(
            makeVec3(-8.0f, 0, 8.0f),
            makeVec3(-8.0f, 0, 2.0f),
            makeVec3(-5.0f, 0, -2.0f),
            makeVec3(-2.0f, 0, -5.0f),
            t
        );
    }

    if (id == 1)
    {
        return bezierPoint(
            makeVec3(0.0f, 0, 9.0f),
            makeVec3(-1.0f, 0, 3.0f),
            makeVec3(0.5f, 0, -2.0f),
            makeVec3(0.0f, 0, -5.4f),
            t
        );
    }

    return bezierPoint(
        makeVec3(8.0f, 0, 7.5f),
        makeVec3(6.0f, 0, 2.0f),
        makeVec3(5.5f, 0, -1.5f),
        makeVec3(2.2f, 0, -5.0f),
        t
    );
}

Vec3 evaluatePathTangent(int id, float t)
{
    Vec3 a = evaluatePath(id, t);
    Vec3 b = evaluatePath(id, t + 0.01f);
    return normalizeVec(sub(b, a));
}

GLuint createCheckerTexture(int size,
    unsigned char r1, unsigned char g1, unsigned char b1,
    unsigned char r2, unsigned char g2, unsigned char b2)
{
    unsigned char* data = new unsigned char[size * size * 3];

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            int checker = ((x / 8) + (y / 8)) % 2;
            int i = (y * size + x) * 3;

            if (checker == 0)
            {
                data[i] = r1;
                data[i + 1] = g1;
                data[i + 2] = b1;
            }
            else
            {
                data[i] = r2;
                data[i + 1] = g2;
                data[i + 2] = b2;
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

        if (bytesRead < (size_t)rowPadded)
        {
            for (int k = (int)bytesRead; k < rowPadded; k++)
                row[k] = 0;
        }

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

void setMatteMaterial()
{
    GLfloat specular[] = { 0.06f, 0.06f, 0.06f, 1.0f };
    GLfloat shininess[] = { 8.0f };

    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

void setPlasticMaterial()
{
    GLfloat specular[] = { 0.55f, 0.55f, 0.55f, 1.0f };
    GLfloat shininess[] = { 65.0f };

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

float daylight()
{
    float d = 1.0f - sunsetAmount;
    if (d < 0.05f) d = 0.05f;
    if (d > 1.0f) d = 1.0f;
    return d;
}

void setupLighting()
{
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHT2);

    float d = daylight();

    GLfloat globalAmbient[] = {
        0.10f + 0.25f * d,
        0.10f + 0.23f * d,
        0.14f + 0.20f * d,
        1.0f
    };

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    if (lightingMode == 1)
    {
        glEnable(GL_LIGHT0);

        GLfloat pos[] = { -0.4f, -1.0f, -0.3f, 0.0f };
        GLfloat diff[] = { 0.75f * d, 0.72f * d, 0.62f * d, 1.0f };
        GLfloat spec[] = { 0.7f, 0.7f, 0.7f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, pos);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
    }
    else if (lightingMode == 2)
    {
        glEnable(GL_LIGHT0);

        GLfloat pos[] = { 0.0f, 8.0f, -5.0f, 1.0f };
        GLfloat diff[] = { 1.0f, 0.78f, 0.45f, 1.0f };
        GLfloat spec[] = { 1.0f, 0.95f, 0.85f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, pos);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.035f);
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.003f);
    }
    else
    {
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);

        GLfloat sunPos[] = { -12.0f + 20.0f * sunsetAmount, 10.0f - 7.0f * sunsetAmount, 6.0f, 1.0f };
        GLfloat sunDiff[] = { 0.95f * d + 0.25f, 0.80f * d + 0.15f, 0.55f * d + 0.10f, 1.0f };
        GLfloat sunSpec[] = { 0.8f, 0.8f, 0.8f, 1.0f };

        glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiff);
        glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpec);

        GLfloat museumPos[] = { 0.0f, 5.0f, -5.0f, 1.0f };
        GLfloat museumDiff[] = {
            0.15f + 0.45f * sunsetAmount,
            0.30f + 0.55f * sunsetAmount,
            0.70f + 0.30f * sunsetAmount,
            1.0f
        };

        glLightfv(GL_LIGHT1, GL_POSITION, museumPos);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, museumDiff);
        glLightfv(GL_LIGHT1, GL_SPECULAR, sunSpec);
    }
}

void drawBox(float sx, float sy, float sz)
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

void drawCylinderY(float radius, float height)
{
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);

    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    gluCylinder(q, radius, radius, height, 28, 12);
    gluDisk(q, 0.0f, radius, 28, 1);

    glTranslatef(0.0f, 0.0f, height);
    gluDisk(q, 0.0f, radius, 28, 1);

    glPopMatrix();

    gluDeleteQuadric(q);
}

void drawCylinderX(float radius, float height)
{
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);

    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glTranslatef(0.0f, 0.0f, -height / 2.0f);

    gluCylinder(q, radius, radius, height, 28, 12);
    gluDisk(q, 0.0f, radius, 28, 1);

    glTranslatef(0.0f, 0.0f, height);
    gluDisk(q, 0.0f, radius, 28, 1);

    glPopMatrix();

    gluDeleteQuadric(q);
}

void drawBitmapText3D(float x, float y, float z, const char* text)
{
    glRasterPos3f(x, y, z);

    for (const char* c = text; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}

void drawScreenText(float x, float y, const char* text)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glColor3f(0.95f, 0.92f, 0.78f);
    glRasterPos2f(x, y);

    for (const char* c = text; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    glEnable(GL_LIGHTING);

    if (texturesEnabled)
        glEnable(GL_TEXTURE_2D);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

void drawGround()
{
    if (texturesEnabled && floorTexture != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
    }

    setMatteMaterial();

    float d = daylight();
    glColor3f(0.42f * d + 0.08f, 0.42f * d + 0.08f, 0.38f * d + 0.10f);

    int size = 60;
    float step = 0.45f;

    for (int i = -size; i < size; i++)
    {
        glBegin(GL_TRIANGLE_STRIP);

        for (int j = -size; j <= size; j++)
        {
            float x1 = i * step;
            float z1 = j * step;
            float y1 = surfaceHeight(x1, z1) - 3.5f;
            Vec3 n1 = surfaceNormal(x1, z1);

            glNormal3f(n1.x, n1.y, n1.z);
            glTexCoord2f(x1 * 0.16f, z1 * 0.16f);
            glVertex3f(x1, y1, z1);

            float x2 = (i + 1) * step;
            float z2 = j * step;
            float y2 = surfaceHeight(x2, z2) - 3.5f;
            Vec3 n2 = surfaceNormal(x2, z2);

            glNormal3f(n2.x, n2.y, n2.z);
            glTexCoord2f(x2 * 0.16f, z2 * 0.16f);
            glVertex3f(x2, y2, z2);
        }

        glEnd();
    }
}

void drawBackdrop()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDepthMask(GL_FALSE);

    if (texturesEnabled && backdropTexture != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, backdropTexture);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
    }

    float d = daylight();
    glColor3f(0.45f + 0.55f * d, 0.48f + 0.50f * d, 0.55f + 0.40f * d);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-90.0f, -15.0f, -45.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(90.0f, -15.0f, -45.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(90.0f, 55.0f, -45.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-90.0f, 55.0f, -45.0f);
    glEnd();

    if (sunsetAmount > 0.75f)
    {
        glDisable(GL_TEXTURE_2D);
        glPointSize(2.0f);
        glColor3f(0.9f, 0.9f, 1.0f);

        glBegin(GL_POINTS);

        for (int i = 0; i < 70; i++)
        {
            float x = -42.0f + (float)((i * 37) % 84);
            float y = 14.0f + (float)((i * 53) % 34);
            glVertex3f(x, y, -44.5f);
        }

        glEnd();
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
}

void drawPathLines()
{
    if (!showGuides) return;

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(5.0f);

    for (int pathId = 0; pathId < 3; pathId++)
    {
        if (pathId == 0) glColor3f(0.0f, 0.65f, 1.0f);
        else if (pathId == 1) glColor3f(0.85f, 0.2f, 0.85f);
        else glColor3f(1.0f, 0.8f, 0.15f);

        glBegin(GL_LINE_STRIP);

        for (int i = 0; i <= 120; i++)
        {
            float t = i / 120.0f;
            Vec3 p = evaluatePath(pathId, t);
            glVertex3f(p.x, p.y - 3.42f, p.z);
        }

        glEnd();
    }

    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

void drawStairsAndRuins()
{
    glDisable(GL_TEXTURE_2D);
    setMatteMaterial();

    glColor3f(0.34f, 0.34f, 0.31f);

    for (int i = 0; i < 7; i++)
    {
        float z = 5.8f - i * 1.35f;
        float y = surfaceHeight(0, z) - 3.15f - i * 0.10f;

        glPushMatrix();
        glTranslatef(0.0f, y, z);
        drawBox(13.0f - i * 0.5f, 0.35f, 1.0f);
        glPopMatrix();
    }

    glColor3f(0.28f, 0.29f, 0.27f);

    for (int i = 0; i < 5; i++)
    {
        float x = -10.0f + i * 5.0f;
        float z = -11.0f;
        float y = surfaceHeight(x, z) - 2.0f;

        glPushMatrix();
        glTranslatef(x, y, z);
        drawBox(1.0f, 4.2f + (i % 2), 1.0f);
        glPopMatrix();
    }

    glColor3f(0.12f, 0.18f, 0.12f);

    for (int i = 0; i < 8; i++)
    {
        float x = -11.0f + (i * 3.1f);
        float z = -9.0f + (i % 3) * 2.5f;
        float y = surfaceHeight(x, z) - 2.8f;

        glPushMatrix();
        glTranslatef(x, y, z);
        drawCylinderY(0.08f, 1.2f);
        glTranslatef(0, 1.2f, 0);
        glutSolidSphere(0.35f, 12, 12);
        glPopMatrix();
    }
}

void drawMuseumSigns()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glColor3f(0.08f, 0.08f, 0.07f);

    glPushMatrix();
    glTranslatef(0.0f, 2.2f, -12.2f);
    drawBox(8.0f, 3.2f, 0.2f);
    glColor3f(0.88f, 0.86f, 0.76f);
    drawBitmapText3D(-3.25f, 0.75f, 0.15f, "THE HUMANS");
    drawBitmapText3D(-3.25f, 0.25f, 0.15f, "ARE GONE");
    drawBitmapText3D(-3.25f, -0.45f, 0.15f, "We study.");
    drawBitmapText3D(-3.25f, -0.85f, 0.15f, "We remember.");
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-8.0f, 0.2f, -4.8f);
    glColor3f(0.10f, 0.08f, 0.06f);
    drawBox(3.5f, 1.6f, 0.18f);
    glColor3f(0.9f, 0.82f, 0.62f);
    drawBitmapText3D(-1.45f, 0.25f, 0.12f, "MUSIC");
    drawBitmapText3D(-1.45f, -0.20f, 0.12f, "Made emotions loud.");
    glPopMatrix();

    glPushMatrix();
    glTranslatef(8.0f, 0.2f, -4.8f);
    glColor3f(0.10f, 0.08f, 0.06f);
    drawBox(3.8f, 1.6f, 0.18f);
    glColor3f(0.9f, 0.82f, 0.62f);
    drawBitmapText3D(-1.55f, 0.25f, 0.12f, "KNOWLEDGE");
    drawBitmapText3D(-1.55f, -0.20f, 0.12f, "Stored in paper.");
    glPopMatrix();

    glEnable(GL_LIGHTING);
}

void drawSkeletonExhibit()
{
    glDisable(GL_TEXTURE_2D);

    float x = 0.0f;
    float z = -6.0f;
    float y = surfaceHeight(x, z) - 3.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.55f, 0.85f, 1.0f, 0.22f);

    glPushMatrix();
    glTranslatef(x, y + 1.8f, z);
    drawBox(3.4f, 3.7f, 1.4f);
    glPopMatrix();

    glDisable(GL_BLEND);

    setMatteMaterial();
    glColor3f(0.90f, 0.84f, 0.70f);

    glPushMatrix();
    glTranslatef(x, y + 0.75f, z);

    glPushMatrix();
    glTranslatef(0, 2.15f, 0);
    glutSolidSphere(0.35f, 24, 24);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0, 1.38f, 0);
    drawBox(0.28f, 0.9f, 0.14f);
    glPopMatrix();

    for (int i = 0; i < 5; i++)
    {
        glPushMatrix();
        glTranslatef(0, 1.72f - i * 0.16f, 0.05f);
        drawBox(0.9f - i * 0.09f, 0.04f, 0.08f);
        glPopMatrix();
    }

    glPushMatrix();
    glTranslatef(-0.48f, 1.45f, 0);
    glRotatef(20, 0, 0, 1);
    drawCylinderY(0.04f, 1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.48f, 1.45f, 0);
    glRotatef(-20, 0, 0, 1);
    drawCylinderY(0.04f, 1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.22f, 0.25f, 0);
    glRotatef(8, 0, 0, 1);
    drawCylinderY(0.05f, 1.1f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.22f, 0.25f, 0);
    glRotatef(-8, 0, 0, 1);
    drawCylinderY(0.05f, 1.1f);
    glPopMatrix();

    glPopMatrix();
}

void drawQuoteSign()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    float x = 0.0f;
    float z = -8.3f;
    float y = surfaceHeight(x, z) - 0.8f;

    glPushMatrix();
    glTranslatef(x, y, z);

    glColor3f(0.07f, 0.06f, 0.05f);
    drawBox(9.4f, 1.45f, 0.14f);

    glColor3f(0.85f, 0.75f, 0.52f);
    drawBitmapText3D(-4.15f, 0.15f, 0.17f, "Weak species.");
    drawBitmapText3D(-4.15f, -0.30f, 0.17f, "Could not survive low battery.");

    glPopMatrix();

    glEnable(GL_LIGHTING);
}

void drawArtifacts()
{
    glDisable(GL_TEXTURE_2D);
    setMetalMaterial();

    float positions[3][2] = {
        {-7.2f, -2.8f},
        {7.2f, -2.8f},
        {0.0f, 2.7f}
    };

    for (int i = 0; i < 3; i++)
    {
        float x = positions[i][0];
        float z = positions[i][1];
        float y = surfaceHeight(x, z) - 2.9f;

        glPushMatrix();
        glTranslatef(x, y, z);

        glColor3f(0.28f, 0.27f, 0.24f);
        drawBox(2.4f, 0.85f, 1.4f);

        glTranslatef(0, 0.72f, 0);

        if (i == 0)
        {
            glColor3f(0.45f, 0.28f, 0.12f);
            drawBox(0.85f, 1.15f, 0.18f);
            glColor3f(0.07f, 0.05f, 0.03f);
            glutSolidTorus(0.04f, 0.23f, 12, 24);
        }
        else if (i == 1)
        {
            glColor3f(0.8f, 0.75f, 0.62f);
            glutSolidTeapot(0.35);
        }
        else
        {
            glColor3f(0.18f, 0.18f, 0.17f);
            drawBox(0.85f, 0.28f, 0.55f);
            glTranslatef(0, 0.22f, 0);
            glColor3f(0.05f, 0.05f, 0.05f);
            drawBox(0.65f, 0.08f, 0.35f);
        }

        glPopMatrix();
    }
}

void drawRobotName(int index)
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    if (index == 0) glColor3f(0.7f, 0.9f, 1.0f);
    else if (index == 1) glColor3f(1.0f, 0.75f, 0.95f);
    else glColor3f(1.0f, 0.9f, 0.45f);

    if (index == 0) drawBitmapText3D(-0.75f, 2.7f, 0.0f, "Odobasian");
    else if (index == 1) drawBitmapText3D(-0.75f, 2.7f, 0.0f, "Dorotheea");
    else drawBitmapText3D(-0.75f, 2.7f, 0.0f, "Talimargen");

    glEnable(GL_LIGHTING);
}

void drawRobot(const RobotState& r, int index)
{
    float R, G, B;

    if (index == 0)
    {
        R = 0.50f;
        G = 0.20f;
        B = 0.55f;
    }
    else if (index == 1)
    {
        R = 0.72f;
        G = 0.25f;
        B = 0.68f;
    }
    else
    {
        R = 0.38f;
        G = 0.18f;
        B = 0.45f;
    }

    float walk = 28.0f * sinf(r.anim * 5.0f);
    float bounce = 0.05f * fabsf(sinf(r.anim * 5.0f));

    if (r.pathT >= 1.0f)
    {
        walk = 0.0f;
        bounce = 0.0f;
    }

    glPushMatrix();

    glTranslatef(r.x, r.y + 0.75f + bounce, r.z);
    glRotatef(r.heading, 0, 1, 0);
    glScalef(r.scale, r.scale, r.scale);

    if (index == 0) setPlasticMaterial();
    else if (index == 1) setMatteMaterial();
    else setMetalMaterial();

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
    drawBox(1.45f, 1.8f, 0.9f);

    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    glTranslatef(0, 1.45f, 0);

    glColor3f(R, G, B);
    drawBox(1.08f, 0.9f, 0.9f);

    glColor3f(0.02f, 0.02f, 0.02f);

    glPushMatrix();
    glTranslatef(-0.25f, 0.12f, 0.48f);
    glutSolidSphere(0.11f, 16, 16);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.25f, 0.12f, 0.48f);
    glutSolidSphere(0.11f, 16, 16);
    glPopMatrix();

    glColor3f(0.1f, 0.9f, 1.0f);

    glPushMatrix();
    glTranslatef(0, -0.22f, 0.50f);
    drawBox(0.44f, 0.05f, 0.04f);
    glPopMatrix();

    glPopMatrix();

    float leftArm = -walk;
    float rightArm = walk;
    float leftZ = 0.0f;
    float rightZ = 0.0f;

    if (r.pathT >= 1.0f)
    {
        leftArm = -18.0f;
        rightArm = -18.0f;
    }

    if (index == 0 && storyStep >= 3)
    {
        rightArm = -65.0f;
        rightZ = -38.0f;
    }

    if (index == 1 && storyStep >= 4)
    {
        leftArm = -35.0f;
        rightArm = -25.0f;
    }

    if (index == 2 && storyStep >= 5)
    {
        leftArm = -105.0f;
        rightArm = -110.0f;
        leftZ = 22.0f * sinf(r.anim * 7.0f);
        rightZ = -22.0f * sinf(r.anim * 7.0f);
    }

    if (storyStep >= 6)
    {
        leftArm = -40.0f + 10.0f * sinf(r.anim * 8.0f);
        rightArm = -40.0f - 10.0f * sinf(r.anim * 8.0f);
    }

    glPushMatrix();
    glTranslatef(-0.95f, 0.55f, 0);
    glRotatef(leftZ, 0, 0, 1);
    glRotatef(leftArm, 1, 0, 0);

    glColor3f(0.25f, 0.25f, 0.28f);
    glutSolidSphere(0.18f, 16, 16);

    glTranslatef(0, -0.55f, 0);
    glColor3f(R, G, B);
    drawBox(0.30f, 0.85f, 0.30f);

    glTranslatef(0, -0.50f, 0);
    glColor3f(0.18f, 0.18f, 0.20f);
    drawBox(0.35f, 0.18f, 0.25f);

    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.95f, 0.55f, 0);
    glRotatef(rightZ, 0, 0, 1);
    glRotatef(rightArm, 1, 0, 0);

    glColor3f(0.25f, 0.25f, 0.28f);
    glutSolidSphere(0.18f, 16, 16);

    glTranslatef(0, -0.55f, 0);
    glColor3f(R, G, B);
    drawBox(0.30f, 0.85f, 0.30f);

    glTranslatef(0, -0.50f, 0);
    glColor3f(0.18f, 0.18f, 0.20f);
    drawBox(0.35f, 0.18f, 0.25f);

    glPopMatrix();

    for (int side = -1; side <= 1; side += 2)
    {
        glPushMatrix();

        glTranslatef(0.42f * side, -1.1f, 0);
        glRotatef(side * walk, 1, 0, 0);

        glColor3f(R, G, B);
        drawBox(0.36f, 0.95f, 0.36f);

        glTranslatef(0, -0.62f, 0.13f);
        glColor3f(0.12f, 0.12f, 0.14f);
        drawBox(0.52f, 0.20f, 0.62f);

        glPopMatrix();
    }

    drawRobotName(index);

    glPopMatrix();

    if (texturesEnabled)
        glEnable(GL_TEXTURE_2D);
}

void drawDialogue()
{
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    const char* line = "Explore mode. Press N for next scene. Press R to restart.";

    if (storyStep == 1)
        line = "The three robots descend into the Museum of Humans.";
    else if (storyStep == 2)
        line = "They stop in front of the skeleton exhibit.";
    else if (storyStep == 3)
        line = "Odobasian: Yes. Weak species. Could not survive low battery.";
    else if (storyStep == 4)
        line = "Dorotheea: They had bones... inside? Bold design choice.";
    else if (storyStep == 5)
        line = "Talimargen: Why is it still smiling?";
    else if (storyStep == 6)
        line = "All robots: Ha. Ha. Ha.";
    else if (storyStep >= 7)
        line = "End of tour. Press R to restart or use 1/2/3/4 to explore.";

    drawScreenText(25.0f, (float)h - 36.0f, line);

    char controls[160];
    sprintf_s(controls, "N next | R restart | 1/2/3/4 cameras | P/O projection | L lights | T textures | G paths | M split | +/- speed");
    drawScreenText(25.0f, 24.0f, controls);
}

void setCamera()
{
    if (storyMode)
    {
        if (storyStep == 1)
        {
            gluLookAt(0.0, 8.0, 20.0, 0.0, 0.0, -5.0, 0.0, 1.0, 0.0);
            return;
        }

        if (storyStep == 2)
        {
            gluLookAt(5.5, 5.0, 5.5, 0.0, -1.4, -6.0, 0.0, 1.0, 0.0);
            return;
        }

        if (storyStep == 3)
        {
            gluLookAt(-4.0, 2.7, -2.9, -2.0, -1.1, -5.0, 0.0, 1.0, 0.0);
            return;
        }

        if (storyStep == 4)
        {
            gluLookAt(0.0, 2.7, -2.4, 0.0, -1.1, -5.4, 0.0, 1.0, 0.0);
            return;
        }

        if (storyStep == 5)
        {
            gluLookAt(4.0, 2.8, -2.5, 2.2, -1.0, -5.0, 0.0, 1.0, 0.0);
            return;
        }

        if (storyStep == 6)
        {
            gluLookAt(0.0, 5.0, 8.5, 0.0, -1.2, -6.0, 0.0, 1.0, 0.0);
            return;
        }

        if (storyStep >= 7)
        {
            gluLookAt(0.0, 9.0, 23.0, 0.0, -1.0, -6.0, 0.0, 1.0, 0.0);
            return;
        }
    }

    if (currentView == 1)
    {
        gluLookAt(0.0, 7.0, 24.0, 0.0, -1.0, -5.0, 0.0, 1.0, 0.0);
    }
    else if (currentView == 2)
    {
        gluLookAt(24.0, 7.0, 0.0, 0.0, -1.0, -5.0, 0.0, 1.0, 0.0);
    }
    else if (currentView == 3)
    {
        gluLookAt(0.0, 30.0, 0.1, 0.0, -2.0, -5.0, 0.0, 0.0, -1.0);
    }
    else
    {
        float yaw = orbitYaw * PI / 180.0f;
        float pitch = orbitPitch * PI / 180.0f;

        float x = orbitDistance * cosf(pitch) * sinf(yaw);
        float y = orbitDistance * sinf(pitch);
        float z = orbitDistance * cosf(pitch) * cosf(yaw);

        gluLookAt(x, y, z, 0.0, -0.8, -5.0, 0.0, 1.0, 0.0);
    }
}

void drawScene()
{
    drawBackdrop();

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    setupLighting();

    drawGround();
    drawStairsAndRuins();
    drawMuseumSigns();
    drawPathLines();
    drawArtifacts();
    drawSkeletonExhibit();
    drawQuoteSign();

    for (int i = 0; i < (int)robots.size(); i++)
        drawRobot(robots[i], i);

    drawDialogue();
}

void renderViewport(int x, int y, int w, int h, int view)
{
    if (h == 0) h = 1;

    glViewport(x, y, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (usePerspective)
        gluPerspective(60.0, (double)w / (double)h, 0.1, 140.0);
    else
        glOrtho(-18.0, 18.0, -12.0, 18.0, -80.0, 90.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    int oldView = currentView;
    currentView = view;

    setCamera();
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
        renderViewport(0, h / 2, w / 2, h / 2, 1);
        renderViewport(w / 2, h / 2, w / 2, h / 2, 2);
        renderViewport(0, 0, w / 2, h / 2, 3);
        renderViewport(w / 2, 0, w / 2, h / 2, 4);
    }
    else
    {
        renderViewport(0, 0, w, h, currentView);
    }

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (usePerspective)
        gluPerspective(60.0, (double)w / (double)h, 0.1, 140.0);
    else
        glOrtho(-18.0, 18.0, -12.0, 18.0, -80.0, 90.0);

    glMatrixMode(GL_MODELVIEW);
}

void resetRobots()
{
    robots.clear();

    robots.push_back({ -8.0f, 0.0f, 8.0f, 0.0f, 0.0033f, 0.0f, 0.0f, 1.00f });
    robots.push_back({ 0.0f, 0.0f, 9.0f, 0.0f, 0.0030f, 0.0f, 1.2f, 0.95f });
    robots.push_back({ 8.0f, 0.0f, 7.5f, 0.0f, 0.0031f, 0.0f, 2.3f, 1.05f });

    storyStep = 0;
    storyMode = false;
    sunsetAmount = 0.0f;
    storyTime = 0.0f;
}

void updateRobots()
{
    for (int i = 0; i < (int)robots.size(); i++)
    {
        RobotState& r = robots[i];

        float targetT = 0.0f;

        if (storyStep <= 0) targetT = 0.0f;
        else if (storyStep == 1) targetT = 0.72f;
        else targetT = 1.0f;

        if (r.pathT < targetT)
        {
            r.pathT += r.speed * (animationSpeed / 0.05f);
            if (r.pathT > targetT)
                r.pathT = targetT;
        }

        Vec3 p = evaluatePath(i, r.pathT);
        Vec3 tangent = evaluatePathTangent(i, r.pathT);

        r.x = p.x;
        r.y = p.y - 2.85f;
        r.z = p.z;

        if (r.pathT < 0.98f)
            r.heading = atan2f(tangent.x, tangent.z) * 180.0f / PI;
        else
            r.heading = atan2f(-r.x, -6.0f - r.z) * 180.0f / PI;

        if (i == 2 && storyStep >= 5)
            r.heading += 35.0f * sinf(storyTime * 4.0f);

        r.anim += animationSpeed;
    }
}

void timer(int value)
{
    updateRobots();

    storyTime += 0.016f * (animationSpeed / 0.05f);

    lightAngle += 1.5f;
    if (lightAngle > 360.0f)
        lightAngle -= 360.0f;

    float d = daylight();

    GLfloat fogColor[4] = {
        0.05f + 0.35f * d,
        0.06f + 0.36f * d,
        0.08f + 0.42f * d,
        1.0f
    };

    glFogfv(GL_FOG_COLOR, fogColor);
    glClearColor(fogColor[0], fogColor[1], fogColor[2], 1.0f);

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int, int)
{
    switch (key)
    {
    case 'n':
    case 'N':
        storyMode = true;
        storyStep++;

        if (storyStep > 7)
            storyStep = 7;

        sunsetAmount = storyStep / 7.0f;

        if (sunsetAmount > 1.0f)
            sunsetAmount = 1.0f;

        break;

    case '1':
        currentView = 1;
        storyMode = false;
        break;

    case '2':
        currentView = 2;
        storyMode = false;
        break;

    case '3':
        currentView = 3;
        storyMode = false;
        break;

    case '4':
        currentView = 4;
        storyMode = false;
        break;

    case 'p':
    case 'P':
        usePerspective = true;
        reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        break;

    case 'o':
    case 'O':
        usePerspective = false;
        reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        break;

    case 'l':
    case 'L':
        lightingMode++;

        if (lightingMode > 3)
            lightingMode = 1;

        break;

    case 't':
    case 'T':
        texturesEnabled = !texturesEnabled;
        break;

    case 'g':
    case 'G':
        showGuides = !showGuides;
        break;

    case 'm':
    case 'M':
        splitScreen = !splitScreen;
        break;

    case '+':
        animationSpeed += 0.02f;

        if (animationSpeed > 0.5f)
            animationSpeed = 0.5f;

        break;

    case '-':
        animationSpeed -= 0.02f;

        if (animationSpeed < 0.01f)
            animationSpeed = 0.01f;

        break;

    case 'r':
    case 'R':
        resetRobots();
        currentView = 4;
        usePerspective = true;
        lightingMode = 3;
        animationSpeed = 0.05f;
        orbitYaw = 45.0f;
        orbitPitch = 24.0f;
        orbitDistance = 30.0f;
        break;

    case 27:
        exit(0);
        break;
    }

    glutPostRedisplay();
}

void specialKeyboard(int key, int, int)
{
    storyMode = false;
    currentView = 4;

    if (key == GLUT_KEY_LEFT)
        orbitYaw -= 3.0f;

    if (key == GLUT_KEY_RIGHT)
        orbitYaw += 3.0f;

    if (key == GLUT_KEY_UP)
        orbitPitch += 2.0f;

    if (key == GLUT_KEY_DOWN)
        orbitPitch -= 2.0f;

    if (orbitPitch > 80.0f)
        orbitPitch = 80.0f;

    if (orbitPitch < -5.0f)
        orbitPitch = -5.0f;

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

        if (orbitDistance < 10.0f)
            orbitDistance = 10.0f;
    }

    if (button == 4)
    {
        orbitDistance += 1.0f;

        if (orbitDistance > 55.0f)
            orbitDistance = 55.0f;
    }

    glutPostRedisplay();
}

void mouseMotion(int x, int y)
{
    if (!mouseDragging)
        return;

    storyMode = false;
    currentView = 4;

    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    orbitYaw += dx * 0.4f;
    orbitPitch += dy * 0.3f;

    if (orbitPitch > 80.0f)
        orbitPitch = 80.0f;

    if (orbitPitch < -5.0f)
        orbitPitch = -5.0f;

    lastMouseX = x;
    lastMouseY = y;

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

    glClearColor(0.45f, 0.55f, 0.70f, 1.0f);

    floorTexture = loadTexture("C:\\Users\\Deborah\\Documents\\Codex\\2026-05-18\\files-mentioned-by-the-user-opengl\\assets\\museum_floor.bmp");
    backdropTexture = loadTexture("C:\\Users\\Deborah\\Documents\\Codex\\2026-05-18\\files-mentioned-by-the-user-opengl\\assets\\museum_backdrop.bmp");

    if (floorTexture == 0)
        floorTexture = createCheckerTexture(64, 80, 80, 70, 45, 42, 35);

    if (backdropTexture == 0)
        backdropTexture = createCheckerTexture(64, 120, 160, 210, 90, 120, 160);

    robotTexture = createCheckerTexture(64, 100, 35, 105, 70, 25, 80);

    glEnable(GL_FOG);

    GLfloat fogColor[4] = { 0.45f, 0.55f, 0.70f, 1.0f };

    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 24.0f);
    glFogf(GL_FOG_END, 80.0f);

    resetRobots();

    printf("Museum of Humans loaded.\n");
    printf("Controls: N next story, R restart, 1/2/3/4 cameras, P/O projection, L lights, T textures, G paths, M split, +/- speed, mouse orbit, ESC exit\n");
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000, 700);
    glutCreateWindow("Museum of Humans - OpenGL Final Project");

    init();

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