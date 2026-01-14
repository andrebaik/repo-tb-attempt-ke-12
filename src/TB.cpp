#include <windows.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// 1. GLOBAL VARIABLES
const float PI = 3.14159265f;
const float MOVE_SPEED = 0.07f;
const float MOUSE_SENSITIVITY = 0.002f;
const float PLAYER_RADIUS = 0.5f;
const float COLLISION_EPSILON = 0.1f;

//transformasi objek
float angle1 = 0.0f;
float tridentAngle = 0.0f;
float tridentLength = 4.5f;
bool isRotating = false;
float crownScale = 1.0f;
float spotCutoff = 45.0f;

// --- STATE MANAGEMENT ---
int gameState = 2;

// Variabel Kamera
float cameraPosX = 0.0f, cameraPosY = 3.5f, cameraPosZ = 8.0f;
float cameraYaw = 0.0f;
float cameraPitch = -20.0f;

// Mouse Control
int lastMouseX = 0, lastMouseY = 0;
bool firstMouse = true;
bool mouseActive = true;
bool keys[256] = {false};

int windowWidth = 1200, windowHeight = 800;

// Variabel Pencahayaan
bool lightingEnabled = true; 
int lightMode = 1;
bool isDirectional = false; 
bool isSpotlight = false; 
float spotExponent = 2.0f; 
float linearAttenuation = 0.0f; 
bool isShiny = true;

// Variabel animasi dan efek
float waterAnimation = 0.0f;
float lightFlicker = 0.0f;
float chandelierRotation = 0.0f;
float statueRotation = 0.0f;
bool dayTime = true;

// Variabel material
GLfloat gold_ambient[] = {0.24725f, 0.1995f, 0.0745f, 1.0f};
GLfloat gold_diffuse[] = {0.75164f, 0.60648f, 0.22648f, 1.0f};
GLfloat gold_specular[] = {0.628281f, 0.555802f, 0.366065f, 1.0f};
GLfloat gold_shininess = 51.2f;

GLfloat marble_ambient[] = {0.8f, 0.8f, 0.8f, 1.0f};
GLfloat marble_diffuse[] = {0.9f, 0.9f, 0.9f, 1.0f};
GLfloat marble_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat marble_shininess = 100.0f;

// 2. STRUCT & COLLISION SYSTEM
struct BoundingBox {
    float minX, maxX, minY, maxY, minZ, maxZ;
};

struct CollisionSphere {
    float x, y, z, radius;
};

// Hanya bounding box interior
BoundingBox insideRoom = {-20, 20, 0.5, 8.5, -20, 20};
BoundingBox insideWalls = {-20.3, 20.3, 0, 9, -20.3, 20.3};

// Posisi untuk 4 objek utama di bagian belakang (sejajar) DI DALAM AREA KARPET
float objectPositions[4][3] = {
    {-11.0f, 0.0f, -12.0f},   // Objek 1: Mahkota (kiri) - DI DALAM KARPET
    {-4.0f, 0.0f, -12.0f},    // Objek 2: Trident (kiri tengah) - DI DALAM KARPET
    {4.0f, 0.0f, -12.0f},     // Objek 3: Katana (kanan tengah) - DI DALAM KARPET
    {11.0f, 0.0f, -12.0f}     // Objek 4: Busur (kanan) - DI DALAM KARPET
};

// Variabel untuk mengontrol posisi busur (objek ke-4)
float bowPosX = 0.0f;  // Translasi X untuk busur
float bowPosZ = 0.0f;  // Translasi Z untuk busur
float bowPosY = 0.0f;  // Translasi Y untuk busur (jika diperlukan)

// Posisi untuk pilar-pilar
float pillarPositions[8][3] = {
    {-18.0f, 0.0f, -18.0f}, {18.0f, 0.0f, -18.0f},
    {-18.0f, 0.0f, 18.0f},  {18.0f, 0.0f, 18.0f},
    {-18.0f, 0.0f, 0.0f},   {18.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, -18.0f},   {0.0f, 0.0f, 18.0f}
};

// 3. COLLISION DETECTION FUNCTIONS (SAMA)
bool checkBoxCollision(float px, float py, float pz, const BoundingBox& box) {
    return (px + PLAYER_RADIUS > box.minX && px - PLAYER_RADIUS < box.maxX &&
            py + PLAYER_RADIUS > box.minY && py - PLAYER_RADIUS < box.maxY &&
            pz + PLAYER_RADIUS > box.minZ && pz - PLAYER_RADIUS < box.maxZ);
}

bool checkAndAdjustCollision(float &newX, float &newY, float &newZ, float oldX, float oldY, float oldZ) {
    bool collisionAdjusted = false;
    
    if (gameState == 2) {
        // Batas ruangan (X dan Z)
        if (newX < insideRoom.minX) { newX = insideRoom.minX + COLLISION_EPSILON; collisionAdjusted = true; }
        if (newX > insideRoom.maxX) { newX = insideRoom.maxX - COLLISION_EPSILON; collisionAdjusted = true; }
        if (newZ < insideRoom.minZ) { newZ = insideRoom.minZ + COLLISION_EPSILON; collisionAdjusted = true; }
        if (newZ > insideRoom.maxZ) {
            if (!(newX > -3 && newX < 3)) { newZ = insideRoom.maxZ - COLLISION_EPSILON; collisionAdjusted = true; }
        }
        
        // Batas lantai dan atap
        float floorHeight = 0.5f;
        float ceilingHeight = 8.5f - PLAYER_RADIUS;
        if (newY < floorHeight) { 
            newY = floorHeight + COLLISION_EPSILON; 
            collisionAdjusted = true; 
        }
        if (newY > ceilingHeight) { 
            newY = ceilingHeight - COLLISION_EPSILON; 
            collisionAdjusted = true; 
        }
    }
    return collisionAdjusted;
}

// 4. PENCAHAYAAN & MATERIAL FUNCTIONS (SAMA)
void initLighting() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);
    
    GLfloat global_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
}

void updateLightingLogic() { 
    if (!lightingEnabled) { 
        glDisable(GL_LIGHTING); 
        return; 
    } 
    glEnable(GL_LIGHTING); 
 
    GLfloat val_ambient[]  = {0.2f, 0.2f, 0.2f, 1.0f}; 
    GLfloat val_diffuse[]  = {0.8f, 0.8f, 0.8f, 1.0f}; 
    GLfloat val_specular[] = {1.0f, 1.0f, 1.0f, 1.0f}; 
    GLfloat val_black[]    = {0.0f, 0.0f, 0.0f, 1.0f}; 
 
    if (lightMode == 1) { 
        glLightfv(GL_LIGHT0, GL_AMBIENT, val_ambient); 
        glLightfv(GL_LIGHT0, GL_DIFFUSE, val_diffuse); 
        glLightfv(GL_LIGHT0, GL_SPECULAR, val_specular); 
    } else if (lightMode == 2) { 
        glLightfv(GL_LIGHT0, GL_AMBIENT, val_ambient); 
        glLightfv(GL_LIGHT0, GL_DIFFUSE, val_black); 
        glLightfv(GL_LIGHT0, GL_SPECULAR, val_black); 
    } else if (lightMode == 3) { 
        glLightfv(GL_LIGHT0, GL_AMBIENT, val_black); 
        glLightfv(GL_LIGHT0, GL_DIFFUSE, val_diffuse); 
        glLightfv(GL_LIGHT0, GL_SPECULAR, val_black); 
    } else if (lightMode == 4) { 
        glLightfv(GL_LIGHT0, GL_AMBIENT, val_black); 
        glLightfv(GL_LIGHT0, GL_DIFFUSE, val_black); 
        glLightfv(GL_LIGHT0, GL_SPECULAR, val_specular); 
    } 
 
    GLfloat w_component = isDirectional ? 0.0f : 1.0f; 
    GLfloat light_pos[] = {0.0f, 5.0f, 0.0f, w_component}; 
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos); 
 
    if (isSpotlight) { 
        GLfloat spotDir[] = {0.0f, -1.0f, 0.0f}; 
        glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotDir); 
        glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, spotCutoff); 
        glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, spotExponent); 
    } else { 
        glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f); 
    } 
 
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f); 
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, linearAttenuation); 
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f); 
 
    if (isShiny) { 
        GLfloat mat_spec[] = {1.0f, 1.0f, 1.0f, 1.0f}; 
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_spec); 
    } else { 
        GLfloat mat_spec[] = {0.0f, 0.0f, 0.0f, 1.0f}; 
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_spec); 
    } 
    
    // Konfigurasi cahaya untuk interior
    GLfloat light0_pos[] = {0.0f, 15.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
    
    GLfloat light1_pos[] = {0.0f, 8.0f, 0.0f, 1.0f};
    GLfloat light1_dir[] = {0.0f, -1.0f, 0.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, light1_dir);
}

void setGoldMaterial() {
    glMaterialfv(GL_FRONT, GL_AMBIENT, gold_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, gold_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, gold_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, gold_shininess);
}

void setMarbleMaterial() {
    glMaterialfv(GL_FRONT, GL_AMBIENT, marble_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, marble_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, marble_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, marble_shininess);
}

void setBrassMaterial() {
    GLfloat brass_ambient[] = {0.329412f, 0.223529f, 0.027451f, 1.0f};
    GLfloat brass_diffuse[] = {0.780392f, 0.568627f, 0.113725f, 1.0f};
    GLfloat brass_specular[] = {0.992157f, 0.941176f, 0.807843f, 1.0f};
    GLfloat brass_shininess = 27.8974f;
    glMaterialfv(GL_FRONT, GL_AMBIENT, brass_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, brass_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, brass_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, brass_shininess);
}

// 5. FUNGSI ELEGAN INTERIOR (SAMA)
void drawCofferedCeiling() {
    glPushMatrix();
    glTranslatef(0.0f, 8.8f, 0.0f);
    glColor3f(0.95f, 0.94f, 0.92f);
    
    float gridSize = 4.0f;
    for(float x = -18.0f; x < 18.0f; x += gridSize) {
        for(float z = -18.0f; z < 18.0f; z += gridSize) {
            glPushMatrix();
            glTranslatef(x + gridSize/2, 0.0f, z + gridSize/2);
            
            // Coffer (indented panel)
            glBegin(GL_QUADS);
            glNormal3f(0, -1, 0);
            glVertex3f(-1.8f, -0.1f, -1.8f);
            glVertex3f(1.8f, -0.1f, -1.8f);
            glVertex3f(1.8f, -0.1f, 1.8f);
            glVertex3f(-1.8f, -0.1f, 1.8f);
            glEnd();
            
            // Rosette in center
            glPushMatrix();
            glTranslatef(0.0f, -0.09f, 0.0f);
            setGoldMaterial();
            glColor3f(0.9f, 0.8f, 0.3f);
            glutSolidSphere(0.3, 16, 16);
            glPopMatrix();
            
            glPopMatrix();
        }
    }
    glPopMatrix();
}

void drawCrystalChandelier() {
    chandelierRotation += 0.1f;
    
    glPushMatrix();
    glTranslatef(0.0f, 8.0f, 0.0f);
    setGoldMaterial();
    
    // Main frame
    glColor3f(0.9f, 0.8f, 0.3f);
    
    // Central stem
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    GLUquadric* q = gluNewQuadric();
    gluCylinder(q, 0.2, 0.1, 1.0, 16, 4);
    gluDeleteQuadric(q);
    glPopMatrix();
    
    // Arms dengan loop
    for(int i = 0; i < 8; i++) {
        glPushMatrix();
        glRotatef(i * 45.0f + chandelierRotation, 0.0f, 1.0f, 0.0f);
        
        // Arm
        glPushMatrix();
        glTranslatef(0.8f, 0.0f, 0.0f);
        glRotatef(30.0f, 0.0f, 0.0f, 1.0f);
        glScalef(1.0f, 0.05f, 0.05f);
        glutSolidCube(1.0f);
        glPopMatrix();
        
        glPopMatrix();
    }
    
    glPopMatrix();
}

void drawWainscoting() {
    // Wood paneling on lower walls
    glColor3f(0.6f, 0.5f, 0.4f);
    float height = 1.2f;
    
    // Loop untuk semua dinding
    float walls[4][4] = {
        {-19.99f, 0.0f, 19.99f, 19.99f}, // North
        {-19.99f, 0.0f, -19.99f, 19.99f}, // South
        {-19.99f, 0.0f, -19.99f, -19.99f}, // West
        {19.99f, 0.0f, -19.99f, 19.99f} // East
    };
    
    for(int i = 0; i < 4; i++) {
        glBegin(GL_QUADS);
        glVertex3f(walls[i][0], walls[i][1], walls[i][2]);
        glVertex3f(walls[i][3], walls[i][1], walls[i][2]);
        glVertex3f(walls[i][3], height, walls[i][2]);
        glVertex3f(walls[i][0], height, walls[i][2]);
        glEnd();
    }
}

// 6. FUNGSI KARPET (HANYA DI DALAM AREA PEMBATAS)
void drawExhibitionRedCarpet() {
    glDisable(GL_LIGHTING);
    glColor3f(0.7f, 0.0f, 0.0f);
    
    // Karpet berbentuk persegi panjang DI DALAM AREA PEMBATAS
    // Area pembatas: dari x=-14 sampai x=14, dari z=-14 sampai z=-8
    float carpetMinX = -13.0f;
    float carpetMaxX = 13.0f;
    float carpetMinZ = -13.0f;
    float carpetMaxZ = -7.0f;
    
    glBegin(GL_QUADS);
    glVertex3f(carpetMinX, 0.01f, carpetMinZ);
    glVertex3f(carpetMaxX, 0.01f, carpetMinZ);
    glVertex3f(carpetMaxX, 0.01f, carpetMaxZ);
    glVertex3f(carpetMinX, 0.01f, carpetMaxZ);
    glEnd();
    
    // Border karpet dengan warna emas
    glColor3f(0.9f, 0.8f, 0.3f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(carpetMinX, 0.011f, carpetMinZ);
    glVertex3f(carpetMaxX, 0.011f, carpetMinZ);
    glVertex3f(carpetMaxX, 0.011f, carpetMaxZ);
    glVertex3f(carpetMinX, 0.011f, carpetMaxZ);
    glEnd();
    
    glEnable(GL_LIGHTING);
}

void drawBarrierPole(float x, float z) {
    GLUquadric* q = gluNewQuadric();
    glColor3f(0.8f, 0.8f, 0.8f);
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    
    // Base
    glPushMatrix(); 
    glRotatef(-90, 1, 0, 0); 
    gluCylinder(q, 0.25, 0.25, 0.15, 20, 1); 
    glPopMatrix();
    
    // Pole
    glPushMatrix(); 
    glRotatef(-90, 1, 0, 0); 
    gluCylinder(q, 0.06, 0.06, 1.8, 15, 1); 
    glPopMatrix();
    
    // Ornamental top
    glPushMatrix(); 
    glTranslatef(0, 1.8, 0); 
    setGoldMaterial();
    glColor3f(0.9f, 0.8f, 0.3f);
    glutSolidSphere(0.12, 15, 15);
    glPopMatrix();
    
    glPopMatrix();
    gluDeleteQuadric(q);
}

void drawRedRope(float x1, float z1, float x2, float z2) {
    glDisable(GL_LIGHTING);
    
    // Rope utama (lurus)
    glColor3f(0.6f, 0.0f, 0.0f);
    glLineWidth(6.0f);
    glBegin(GL_LINES);
    glVertex3f(x1, 1.65f, z1);
    glVertex3f(x2, 1.65f, z2);
    glEnd();
    
    glEnable(GL_LIGHTING);
}

// 7. FUNGSI PEDESTAL & DISPLAY CASE UNTUK SEMUA OBJEK
void drawDisplayCase(float x, float z) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glPopMatrix();
}

void drawPodium(float x, float z) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    
    // Podium base (hitam) - SAMA SEPERTI YANG ADA DI KATANA
    glPushMatrix(); 
    glColor3f(0.2f, 0.2f, 0.2f); 
    glTranslatef(0.0f, 1.0f, 0.0f); 
    glScalef(2.0f, 2.0f, 2.0f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    
    // White display area on top of podium - SAMA SEPERTI YANG ADA DI KATANA
    glPushMatrix();
    glColor3f(0.9f, 0.9f, 0.9f);
    glTranslatef(0.0f, 2.1f, 0.0f);
    glScalef(1.9f, 0.1f, 1.9f);
    glutSolidCube(1.0f);
    glPopMatrix();
    
    glPopMatrix();
}

// ==================== FUNGSI OBJEK SPESIFIK ====================

// 1. MAHKOTA (CROWN) - dari drawCrown() yang sudah ada
void drawCrownDisplay(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(crownScale, crownScale, crownScale);
    
    GLUquadric* q = gluNewQuadric();
    GLfloat no_specular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, no_specular);
    
    // Base sphere
    glPushMatrix();
    glColor3f(0.45f, 0.0f, 0.05f); 
    glTranslatef(0.0f, 0.4f, 0.0f);
    glScalef(1.15f, 0.9f, 1.15f); 
    glutSolidSphere(0.7, 40, 40);
    glPopMatrix();
    
    // Gold rings
    glColor3f(0.85f, 0.65f, 0.1f); 
    for (float h = 0.0f; h <= 0.3f; h += 0.15f) {
        glPushMatrix(); 
        glTranslatef(0.0f, h, 0.0f); 
        glRotatef(90, 1, 0, 0);
        glutSolidTorus(0.06, 0.85, 15, 50); 
        glPopMatrix();
    }
    
    // Spikes with jewels
    for (int i = 0; i < 8; i++) {
        glPushMatrix(); 
        glRotatef(i * 45.0f, 0.0f, 1.0f, 0.0f); 
        glTranslatef(0.85f, 0.25f, 0.0f);
        
        // Spike base
        glColor3f(0.85f, 0.65f, 0.1f); 
        glPushMatrix(); 
        glScalef(0.15f, 0.5f, 0.3f); 
        glutSolidSphere(0.6, 15, 15); 
        glPopMatrix();
        
        // Jewel
        glTranslatef(0.1f, 0.0f, 0.0f); 
        if (i % 2 == 0) {
            glColor3f(0.9f, 0.0f, 0.1f); // Ruby
        } else {
            glColor3f(0.0f, 0.2f, 0.8f); // Sapphire
        }
        glutSolidSphere(0.14, 15, 15); 
        glPopMatrix();
    }
    
    // Decorative arcs
    glColor3f(0.85f, 0.65f, 0.1f);
    for (int i = 0; i < 4; i++) {
        glPushMatrix(); 
        glRotatef(i * 90.0f, 0.0f, 1.0f, 0.0f);
        for (float a = 0; a <= PI; a += 0.1f) {
            float x_pos = cos(a) * 0.85f; 
            float y_pos = sin(a) * 1.3f + 0.15f; 
            glPushMatrix(); 
            glTranslatef(x_pos, y_pos, 0); 
            glutSolidSphere(0.09, 10, 10); 
            glPopMatrix();
        }
        glPopMatrix();
    }
    
    // Top ornament
    glPushMatrix(); 
    glTranslatef(0.0f, 1.45f, 0.0f); 
    glColor3f(0.85f, 0.65f, 0.1f);
    glutSolidSphere(0.25, 20, 20); 
    
    glTranslatef(0.0f, 0.35f, 0.0f); 
    glColor3f(1.0f, 1.0f, 1.0f); 
    
    // Cross
    glPushMatrix(); 
    glScalef(0.1f, 0.45f, 0.1f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    
    glPushMatrix(); 
    glScalef(0.35f, 0.1f, 0.1f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    glPopMatrix(); 
    
    gluDeleteQuadric(q);
    glPopMatrix();
}

void drawTridentDisplay(float x, float y, float z) {
    GLUquadric* q = gluNewQuadric();
        glPushMatrix(); 
        glTranslatef(x, y, z);
        glRotatef(tridentAngle, 0.0f, 1.0f, 0.0f);
        
        glRotatef(.0f, 0.0f, 1.0f, 0.0f); 
        glRotatef(-30.0f, 1.0f, 0.0f, 0.0f); 
        glRotatef(16.0f, 1.0f, 0.0f, 0.0f);
        
        glColor3f(0.4f, 0.2f, 0.1f); 
        gluCylinder(q, 0.08f, 0.08f, tridentLength, 16, 4);
        glTranslatef(0.0f, 0.0f, tridentLength); 
        glColor3f(0.8f, 0.8f, 0.9f);
        
        glPushMatrix(); 
        gluCylinder(q, 0.06f, 0.0f, 1.2f, 16, 4); 
        glPopMatrix();
        
        glPushMatrix();
        glTranslatef(-0.3f, 0.0f, 0.0f); 
        glRotatef(-15.0f, 0.0f, 1.0f, 0.0f);
        gluCylinder(q, 0.05f, 0.0f, 1.0f, 16, 4); 
        glPopMatrix();
        
        glPushMatrix(); 
        glTranslatef(0.3f, 0.0f, 0.0f); 
        glRotatef(15.0f, 0.0f, 1.0f, 0.0f); 
        gluCylinder(q, 0.05f, 0.0f, 1.0f, 16, 4); 
        glPopMatrix();
        
        glTranslatef(-0.45f, 0.0f, 0.0f); 
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f); 
        glScalef(1.0f, 1.0f, 0.2f); 
        gluCylinder(q, 0.08f, 0.08f, 4.5f, 16, 4);
    
        glPopMatrix(); 
    gluDeleteQuadric(q);
}


// 3. KATANA - dari drawMuseumKatana() yang sudah ada
void drawKatanaDisplay(float x, float y, float z) {
    glPushMatrix(); 
    glTranslatef(x, y, z); 
    glRotatef(-60.0f, 0.0f, 1.0f, 0.0f);
    
    // Sword holder prongs
    glPushMatrix(); 
    glTranslatef(0.0f, 2.0f, 0.0f); 
    glPushMatrix(); 
    glColor3f(0.4f, 0.2f, 0.0f); 
    glTranslatef(-0.5f, 0.3f, 0.0f); 
    glScalef(0.1f, 0.6f, 0.3f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    glPushMatrix(); 
    glColor3f(0.4f, 0.2f, 0.0f); 
    glTranslatef(0.5f, 0.3f, 0.0f); 
    glScalef(0.1f, 0.6f, 0.3f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    glPopMatrix();
    
    // Katana sword
    glPushMatrix(); 
    glTranslatef(0.0f, 2.5f, 0.0f); 
    glRotatef(5.0f, 0.0f, 0.0f, 1.0f); 
    
    // Blade
    glPushMatrix(); 
    glColor3f(0.9f, 0.9f, 0.95f); 
    glScalef(4.0f, 0.05f, 0.1f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    
    // Hilt (tsuka)
    glPushMatrix(); 
    glColor3f(0.6f, 0.0f, 0.0f); 
    glTranslatef(2.3f, 0.0f, 0.0f); 
    glScalef(1.2f, 0.08f, 0.12f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    
    // Guard (tsuba)
    glPushMatrix(); 
    glColor3f(1.0f, 0.8f, 0.0f); 
    glTranslatef(1.8f, 0.0f, 0.0f); 
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f); 
    glutSolidTorus(0.02, 0.15, 10, 20); 
    glPopMatrix();
    glPopMatrix(); 
    
    glPopMatrix(); 
}

// 4. BUSUR DAN PANAH - dari BusurDanPanah() dan fungsi terkait
void KurvaBusur(float R, float baseTube) {
    const int curveSteps = 80;
    const int circleSteps = 24;

    for(int i = 0; i < curveSteps; i++) {
        float t1 = -M_PI/2 + i * M_PI / curveSteps;
        float t2 = -M_PI/2 + (i+1) * M_PI / curveSteps;

        // Tebal bertambah di ujung
        float k1 = fabs(sin(t1));
        float k2 = fabs(sin(t2));
        float tube1 = baseTube + 0.02f * k1;
        float tube2 = baseTube + 0.02f * k2;

        float x1 = R * cos(t1), y1 = R * sin(t1);
        float x2 = R * cos(t2), y2 = R * sin(t2);

        float nx1 = -sin(t1), ny1 = cos(t1);
        float nx2 = -sin(t2), ny2 = cos(t2);

        glBegin(GL_QUAD_STRIP);
        for(int j = 0; j <= circleSteps; j++) {
            float a = 2 * M_PI * j / circleSteps;

            float cx1 = tube1 * cos(a);
            float cz1 = tube1 * sin(a);

            float cx2 = tube2 * cos(a);
            float cz2 = tube2 * sin(a);

            glVertex3f(x1 + nx1 * cx1, y1 + ny1 * cx1, cz1);
            glVertex3f(x2 + nx2 * cx2, y2 + ny2 * cx2, cz2);
        }
        glEnd();
    }
}

void BuluPanah() {
    glBegin(GL_TRIANGLES);
    glColor3f(1.0, 1.0, 1.0);
    
    glVertex3f(0.0f, 2.0f, 4.0f);
    glVertex3f(0.05f, 0.0f, 2.0f);
    glVertex3f(0.05f, 0.0f, 0.0f);
    
    glVertex3f(0.0f, 2.0f, 4.0f);
    glVertex3f(-0.05f, 0.0f, 2.0f);
    glVertex3f(-0.05f, 0.0f, 0.0f);
    
    glVertex3f(0.05f,0.0f,2.0f);
    glVertex3f(-0.05f,0.0f,2.0f);
    glVertex3f(0.0f,2.0f,4.0f);
    
    glEnd();
}

void Busur() {
    float R = 0.7f;
    float tube = 0.03f;
    
    glColor3f(0.1f,0.1f,0.1f);
    KurvaBusur(R, tube);
	
    glColor3f(1,1,1);
    glBegin(GL_LINES);
        glVertex3f(0, -R, 0);
        glVertex3f(0,  R, 0);
    glEnd();
}

void Panah() {
	// Bagian tengah (cone)
    glColor3f(0.8f, 0.6f, 0.4f); // Warna kayu
    glPushMatrix();
    	glTranslatef(0.0f, 2.0f, 2.0f);
    	glScalef(0.05, 0.1, 0.1);
    	glutSolidCone(1.0, 2.0, 10, 10); 
    glPopMatrix();
    // Bagian panah (cylinder)
    glColor3f(0.9f, 0.9f, 0.9f); // Warna silver
    glPushMatrix();
    	glTranslatef(0.0f, 2.0f, -2.0f);
    	glScalef(0.02f, 0.02f, 2.0f);
    	glutSolidCylinder(1.0f,2.0f,20.0f,15.0f); 
    glPopMatrix();
    // Bulu busur (3 bagian dengan rotasi berbeda)
    glColor3f(1.0f, 1.0f, 0.8f); // Warna putih kekuningan
    // Bulu busur 1
    glPushMatrix();
    	glTranslatef(0.0f, 2.0f, -1.9f);
    	glScalef(0.1f,0.1f,0.1f);
    	glRotatef(0.0f,0.0f,0.0f,1.0f);
    	glRotatef(180.0f,0.0f,1.0f,0.0f);
	BuluPanah();
    glPopMatrix();
    // Bulu busur 2 (rotasi 120 derajat)
    glPushMatrix();
    	glTranslatef(0.0f, 2.0f, -1.9f);
		glScalef(0.1f,0.1f,0.1f);
    	glRotatef(120.0f, 0.0f, 0.0f, 1.0f);
    	glRotatef(180.0f,0.0f,1.0f,0.0f);
    	BuluPanah();
    glPopMatrix();
    // Bulu busur 3 (rotasi 240 derajat)
    glPushMatrix();
    	glTranslatef(0.0f, 2.0f, -1.9f);
    	glScalef(0.1f,0.1f,0.1f);
    	glRotatef(240.0f, 0.0f, 0.0f, 1.0f);
    	glRotatef(180.0f,0.0f,1.0f,0.0f);
    	BuluPanah();
    glPopMatrix();
}
 
void BusurDanPanah() { 
    glPushMatrix(); // Push matrix utama untuk objek
    // Rotasi dan posisi objek
    glTranslatef(0.0f, 0.0f, 0.0f);
    //Busurnya
    glPushMatrix();
    glTranslatef(0.0,1.0,0.0);
    glRotatef(270.0,0.0,1.0,0.0);
    glScalef(1.5,1.5,1.5);
    Busur();
    glPopMatrix();
    //Panahnya
    glPushMatrix();
    glTranslatef(0.0,0.0,0.5);
    glScalef(0.5,0.5,0.5);
    Panah();
    glPopMatrix();
    glPopMatrix(); // Pop matrix utama
}

void drawBowAndArrowDisplay(float x, float y, float z) {
	glPushMatrix();
	glTranslatef(x + bowPosX, y + bowPosY, z + bowPosZ);
	BusurDanPanah();
	glPopMatrix(); 
}

// 8. FUNGSI PILLAR
void drawDecorativePillar(float x, float z, float height = 8.0f) {
    GLUquadric* q = gluNewQuadric();
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    
    setMarbleMaterial();
    glColor3f(0.95f, 0.93f, 0.88f); // Warna marble
    
    // Base
    glPushMatrix();
    glScalef(1.2f, 0.3f, 1.2f);
    glutSolidCube(1.0f);
    glPopMatrix();
    
    // Column shaft
    glPushMatrix();
    glTranslatef(0.0f, 0.3f, 0.0f);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.4, 0.35, height, 20, 8);
    glPopMatrix();
    
    // Capital (top decoration)
    glPushMatrix();
    glTranslatef(0.0f, height, 0.0f);
    
    // Abacus
    glPushMatrix();
    glScalef(0.8f, 0.2f, 0.8f);
    glutSolidCube(1.0f);
    glPopMatrix();
    
    // Ornament
    setGoldMaterial();
    glColor3f(0.9f, 0.8f, 0.3f);
    for(int i = 0; i < 4; i++) {
        glPushMatrix();
        glRotatef(i * 90.0f, 0.0f, 1.0f, 0.0f);
        glTranslatef(0.4f, 0.1f, 0.0f);
        glutSolidSphere(0.15, 10, 10);
        glPopMatrix();
    }
    
    glPopMatrix();
    
    glPopMatrix();
    gluDeleteQuadric(q);
}

// 9. FUNGSI PINTU INTERIOR
void drawInteriorDoor() {
    glPushMatrix();
    glTranslatef(0.0f, 3.5f, 19.8f);
    
    // Door frame interior
    glColor3f(0.25f, 0.2f, 0.15f);
    glPushMatrix();
    glScalef(6.4f, 7.4f, 0.4f);
    glutSolidCube(1.0f);
    glPopMatrix();
    
    glPopMatrix();
}

// 10. FUNGSI LUKISAN
void drawVerticalPainting(float x, float z, float width, float height) {
    // Frame painting vertikal
    glPushMatrix(); 
    glTranslatef(x, height/2 + 1.0f, z); 
    
    // Frame kayu tebal
    glColor3f(0.5f, 0.3f, 0.1f); 
    glPushMatrix(); 
    glScalef(width, height, 0.2f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    
    // Canvas painting
    glPushMatrix(); 
    glColor3f(0.9f, 0.85f, 0.7f); // Warna canvas kuning tua
    glTranslatef(0.0f, 0.0f, 0.11f); 
    glScalef(width * 0.9f, height * 0.9f, 0.01f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    
    // Detail frame
    glColor3f(0.7f, 0.5f, 0.2f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-width/2, -height/2, 0.101f);
    glVertex3f(width/2, -height/2, 0.101f);
    glVertex3f(width/2, height/2, 0.101f);
    glVertex3f(-width/2, height/2, 0.101f);
    glEnd();
    
    // Picture light di atas
    glPushMatrix();
    glTranslatef(0.0f, height/2 + 0.2f, 0.15f);
    glColor3f(0.8f, 0.8f, 0.8f);
    glScalef(width * 0.8f, 0.1f, 0.1f);
    glutSolidCube(1.0f);
    
    // Bulb lampu
    glTranslatef(0.0f, -0.15f, 0.0f);
    glColor3f(1.0f, 1.0f, 0.8f);
    glutSolidSphere(0.08, 10, 10);
    glPopMatrix();
    
    glPopMatrix();
}

void drawPictureFrame() {
    // Lukisan kiri vertikal (lebih tinggi)
    drawVerticalPainting(-7.0f, -19.9f, 3.0f, 5.0f);
    
    // Lukisan kanan vertikal (lebih pendek)
    drawVerticalPainting(7.0f, -19.9f, 3.0f, 4.0f);
}

// 11. SCENE UTAMA DENGAN LOOPING
void drawMarbleFloor() {
    glDisable(GL_LIGHTING);
    
    float tileSize = 2.0f;
    for(float x = -20.0f; x < 20.0f; x += tileSize) {
        for(float z = -20.0f; z < 20.0f; z += tileSize) {
            if(((int)(x/tileSize) + (int)(z/tileSize)) % 2 == 0) {
                glColor3f(0.95f, 0.95f, 0.92f);
            } else {
                glColor3f(0.85f, 0.85f, 0.82f);
            }
            
            glBegin(GL_QUADS);
            glVertex3f(x, 0.001f, z);
            glVertex3f(x + tileSize, 0.001f, z);
            glVertex3f(x + tileSize, 0.001f, z + tileSize);
            glVertex3f(x, 0.001f, z + tileSize);
            glEnd();
        }
    }
    glEnable(GL_LIGHTING);
}

void drawInteriorSceneFull() {
    glDisable(GL_CULL_FACE);
    
    // Marble floor
    drawMarbleFloor();
    
    // Walls
    float s = 20.0f, h = 9.0f; 
    glColor3ub(240, 235, 230); // Cream color
    glBegin(GL_QUADS); 
    // West wall
    glVertex3f(-s,0,-s); glVertex3f(-s,0,s); glVertex3f(-s,h,s); glVertex3f(-s,h,-s);
    // East wall
    glVertex3f(s,0,-s); glVertex3f(s,0,s); glVertex3f(s,h,s); glVertex3f(s,h,-s);
    // South wall
    glVertex3f(-s,0,-s); glVertex3f(s,0,-s); glVertex3f(s,h,-s); glVertex3f(-s,h,-s);
    // North wall (dengan pintu)
    glVertex3f(-s,0,s); glVertex3f(-5,0,s); glVertex3f(-5,h,s); glVertex3f(-s,h,s);
    glVertex3f(5,0,s); glVertex3f(s,0,s); glVertex3f(s,h,s); glVertex3f(5,h,s);
    glVertex3f(-5,7,s); glVertex3f(5,7,s); glVertex3f(5,h,s); glVertex3f(-5,h,s);
    glEnd();
    
    // Ceiling
    drawCofferedCeiling();
    
    // Wainscoting
    drawWainscoting();
    
    // Chandelier
    drawCrystalChandelier();
    
    // Interior door
    drawInteriorDoor();
    
    // Lukisan di dinding
    drawPictureFrame();
    
    // Karpet HANYA DI DALAM AREA PEMBATAS
    drawExhibitionRedCarpet();
    
    glEnable(GL_CULL_FACE);
    
// ========== BARRIER POLES DAN ROPES (LURUS) ==========
// Posisi barrier poles mengelilingi area karpet
float barrierPoles[10][2] = {
    {-14.0f, -14.0f}, {-7.0f, -14.0f}, {0.0f, -14.0f}, {7.0f, -14.0f}, {14.0f, -14.0f},
    {-14.0f, -6.0f},  {-7.0f, -6.0f},  {0.0f, -6.0f},  {7.0f, -6.0f},  {14.0f, -6.0f}
};

// Gambar semua poles dengan loop
for(int i = 0; i < 10; i++) {
    drawBarrierPole(barrierPoles[i][0], barrierPoles[i][1]);
}
    
// Gambar ropes (lurus) dengan loop - membentuk persegi panjang
for(int i = 0; i < 4; i++) {
    // Ropes horizontal bawah
    drawRedRope(barrierPoles[i][0], barrierPoles[i][1],
                barrierPoles[i+1][0], barrierPoles[i+1][1]);

    // Ropes horizontal atas
    drawRedRope(barrierPoles[i+5][0], barrierPoles[i+5][1],
                barrierPoles[i+6][0], barrierPoles[i+6][1]);
}
   
// Ropes vertikal
for(int i = 0; i < 5; i++) {
    drawRedRope(barrierPoles[i][0], barrierPoles[i][1],
                barrierPoles[i+5][0], barrierPoles[i+5][1]);
}

    // ========== PILLARS (DEKORATIF) ==========
    // Gambar semua pilar dengan loop
    for(int i = 0; i < 7; i++) {
        drawDecorativePillar(pillarPositions[i][0], pillarPositions[i][2]);
    }
    
    // ========== 4 OBJEK UTAMA DI DALAM AREA PEMBATAS (SEJAJAR) ==========
    // Gambar 4 objek utama dengan loop
    for(int i = 0; i < 4; i++) {
        float x = objectPositions[i][0];
        float z = objectPositions[i][2];
        
        // 1. Gambar display case untuk setiap objek (SAMA SEPERTI KATANA)
        drawDisplayCase(x, z);
        
        // 2. Gambar podium untuk setiap objek (SAMA SEPERTI KATANA)
        drawPodium(x, z);
        
        // 3. Gambar objek yang berbeda di atas podium
        switch(i) {
            case 0: // Objek 1: Mahkota
                drawCrownDisplay(x, 2.5f, z);
                break;
            case 1: // Objek 2: Trident
                drawTridentDisplay(x, 2.5f, z);
                break;
            case 2: // Objek 3: Katana
                drawKatanaDisplay(x, 0.0f, z);
                break;
            case 3: // Objek 4: Busur dan Panah
                // Menggunakan variabel bowPosX dan bowPosZ untuk translasi
                drawBowAndArrowDisplay(x, 2.5f, z);
                break;
        }
    }
}

// 12. SYSTEM FUNCTIONS (SAMA)
void updateCamera() {
    float yawRad = cameraYaw * PI / 180.0f; 
    float pitchRad = cameraPitch * PI / 180.0f;
    float forwardX = sin(yawRad) * cos(pitchRad); 
    float forwardY = -sin(pitchRad); 
    float forwardZ = -cos(yawRad) * cos(pitchRad);
    float rightX = sin(yawRad + PI/2); 
    float rightZ = -cos(yawRad + PI/2);
    
    float oldX = cameraPosX, oldY = cameraPosY, oldZ = cameraPosZ;
    float newX = cameraPosX, newY = cameraPosY, newZ = cameraPosZ;
    
    if (keys['w'] || keys['W']) { 
        newX += forwardX * MOVE_SPEED; 
        newY += forwardY * MOVE_SPEED; 
        newZ += forwardZ * MOVE_SPEED; 
    }
    if (keys['s'] || keys['S']) { 
        newX -= forwardX * MOVE_SPEED; 
        newY -= forwardY * MOVE_SPEED; 
        newZ -= forwardZ * MOVE_SPEED; 
    }
    if (keys['a'] || keys['A']) { 
        newX -= rightX * MOVE_SPEED; 
        newZ -= rightZ * MOVE_SPEED; 
    }
    if (keys['d'] || keys['D']) { 
        newX += rightX * MOVE_SPEED; 
        newZ += rightZ * MOVE_SPEED; 
    }
    if (keys[' ']) { newY += MOVE_SPEED; }
    if (keys['c'] || keys['C']) { newY -= MOVE_SPEED; }
    
    checkAndAdjustCollision(newX, newY, newZ, oldX, oldY, oldZ);
    
    cameraPosX = newX; 
    cameraPosY = newY; 
    cameraPosZ = newZ;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
    gluPerspective(60.0, (double)windowWidth / windowHeight, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW); 
    glLoadIdentity();
    
    float yawRad = cameraYaw * PI / 180.0f; 
    float pitchRad = cameraPitch * PI / 180.0f;
    float lookX = cameraPosX + sin(yawRad) * cos(pitchRad); 
    float lookY = cameraPosY - sin(pitchRad); 
    float lookZ = cameraPosZ - cos(yawRad) * cos(pitchRad);
    
    gluLookAt(cameraPosX, cameraPosY, cameraPosZ, lookX, lookY, lookZ, 0.0f, 1.0f, 0.0f);
    
    // Update animations
    angle1 += 1.1f; 
    if (angle1 > 360.0f) angle1 -= 360.0f;
    chandelierRotation += 0.05f;
    
    updateLightingLogic();
    
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f); 
    drawInteriorSceneFull(); 
    
    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) { 
    keys[key] = true; 
    if (key == 27) exit(0);
    
    if (key == '1') lightMode = 1; 
    if (key == '2') lightMode = 2; 
    if (key == '3') lightMode = 3; 
    if (key == '4') lightMode = 4; 
    if (key == 'j' || key == 'J') { 
        spotCutoff -= 2.0f; 
        if (spotCutoff < 0) spotCutoff = 0; 
    } 
    if (key == 'k' || key == 'K') { 
        spotCutoff += 2.0f; 
        if (spotCutoff > 90) spotCutoff = 90; 
    } 
    
    if (key == 'n' || key == 'N') linearAttenuation += 0.05f; 
    if (key == 'm' || key == 'M') { 
        linearAttenuation -= 0.05f; 
        if (linearAttenuation < 0) linearAttenuation = 0; 
    } 
    
    if(key == 'L' || key == 'l') {
        tridentAngle += 5.0f;
        if(tridentAngle > 360.0f) tridentAngle -= 360.0f;
    }
    if(key == ';') {   // tambah panjang
        tridentLength += 0.2f;
    }
    if(key == ':') {   // kurang panjang
        tridentLength -= 0.2f;
        if(tridentLength < 0.5f) tridentLength = 0.5f; // batas minimal
    }

    if (key == 'k' || key == 'K') {
        crownScale += 0.1f;
        if (crownScale > 70.0f) crownScale = 70.0f;
    }

    if (key == 'j' || key == 'J') {
        crownScale -= 0.1f;
        if (crownScale < 0.01f) crownScale = 0.01f;
    }
    if (key == 'f' || key == 'F') {
        bowPosX -= 0.1f;  // Geser ke kiri
    }
    if (key == 'h' || key == 'H') {
        bowPosX += 0.1f;  // Geser ke kanan
    }
    if (key == 't' || key == 'T') {
        bowPosZ -= 0.1f;  // Geser ke belakang
    }
    if (key == 'g' || key == 'G') {
        bowPosZ += 0.1f;  // Geser ke depan
    }
    if (key == 'r' || key == 'R') { // Reset posisi objek (busur) ke posisi awal (tombol 'r' atau 'R')
        bowPosX = 0.0f;
        bowPosZ = 0.0f;
        bowPosY = 0.0f;
    }
    
    glutPostRedisplay();
}

void keyboardUp(unsigned char key, int x, int y) { 
    keys[key] = false; 
}

void mouseMotion(int x, int y) {
    int centerX = windowWidth / 2, centerY = windowHeight / 2;
    if (x == centerX && y == centerY) return;
    
    cameraYaw += (x - centerX) * MOUSE_SENSITIVITY * 50.0f; 
    cameraPitch += (y - centerY) * MOUSE_SENSITIVITY * 50.0f;
    
    if (cameraPitch > 89.0f) cameraPitch = 89.0f; 
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;
    
    glutWarpPointer(centerX, centerY);
}

void reshape(int w, int h) {
    windowWidth = w; 
    windowHeight = h; 
    glViewport(0, 0, w, h);
}

void idle() {
    updateCamera(); 
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Grand Museum - 4 Artefak Sejarah dengan Display Case Seragam");
    glutFullScreen();
    initLighting();
    glutDisplayFunc(display); 
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard); 
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseMotion); 
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}