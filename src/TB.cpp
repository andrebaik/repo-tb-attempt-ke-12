#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// ========================================================================
// 1. GLOBAL VARIABLES
// ========================================================================
const float PI = 3.14159265f;
const float MOVE_SPEED = 0.07f;
const float MOUSE_SENSITIVITY = 0.002f;
const float PLAYER_RADIUS = 0.5f;
const float COLLISION_EPSILON = 0.1f; // Jarak minimum dari dinding
float angle1 = 0.0f;

// --- STATE MANAGEMENT ---
int gameState = 0; 
float loadingProgress = 0.0f;

// Variabel Kamera
float cameraPosX = 0.0f, cameraPosY = 3.5f, cameraPosZ = 45.0f; 
float cameraYaw = -90.0f; 
float cameraPitch = 0.0f;

// Mouse Control
int lastMouseX = 0, lastMouseY = 0;
bool firstMouse = true;
bool mouseActive = true;
bool keys[256] = {false};

int windowWidth = 1200, windowHeight = 800;

// Variabel Pencahayaan
bool lightingEnabled = true;

// ========================================================================
// 2. STRUCT & COLLISION SYSTEM
// ========================================================================
struct BoundingBox {
    float minX, maxX, minY, maxY, minZ, maxZ;
};

struct CollisionSphere {
    float x, y, z, radius;
};

// Bounding boxes untuk objek luar
BoundingBox outsideBounds = {-50, 50, 0.5, 8, -50, 50}; // Batas area luar
BoundingBox buildingBox = {-16, 16, 0, 12, -20, 10}; // Gedung utama
BoundingBox doorBoxOut = {-3, 3, 1, 8, 9.8, 10.2}; // Pintu luar
BoundingBox frontArea = {-50, 50, 0, 50, 8, 50}; // Area depan museum saja (tidak boleh ke belakang)
BoundingBox noGoBehind = {-20, 20, 0, 20, -100, 9}; // Area di belakang museum DILARANG

// Bounding boxes untuk objek dalam
BoundingBox insideRoom = {-20, 20, 0.5, 8.5, -20, 20}; // Ruangan utama (dengan margin)
BoundingBox insideWalls = {-20.3, 20.3, 0, 9, -20.3, 20.3}; // Dinding sebenarnya
BoundingBox katanaBox = {-13.5, -6.5, 0, 6, -13.5, -6.5}; // Katana display
BoundingBox opikBox = {-12.5, -7.5, 0, 3, 7.5, 12.5}; // Area Opik
BoundingBox rezaBox = {7.5, 12.5, 0, 3, 7.5, 12.5}; // Area Reza
BoundingBox andreBox = {7.5, 12.5, 0, 3, -12.5, -7.5}; // Area Andre
BoundingBox potBox = {-20.5, -17.5, 0, 3, -1.5, 1.5}; // Pot bunga
BoundingBox pictureBoxLeft = {-11.5, -2.5, 2, 8, -20.2, -19.8}; // Lukisan kiri
BoundingBox pictureBoxRight = {2.5, 11.5, 2, 8, -20.2, -19.8}; // Lukisan kanan

// Pilar-pilar luar
std::vector<CollisionSphere> pillars;

void initPillars() {
    for (int i = 0; i < 6; i++) {
        CollisionSphere pillar;
        pillar.x = -10.0f + i * 4.0f;
        pillar.y = 4.5f;
        pillar.z = 10.0f;
        pillar.radius = 0.8f; // Radius lebih besar untuk margin
        pillars.push_back(pillar);
    }
}

// ========================================================================
// 3. COLLISION DETECTION FUNCTIONS YANG DIPERBAIKI
// ========================================================================
bool checkBoxCollision(float px, float py, float pz, const BoundingBox& box) {
    // Periksa collision dengan bounding box dengan margin
    return (px + PLAYER_RADIUS > box.minX && px - PLAYER_RADIUS < box.maxX &&
            py + PLAYER_RADIUS > box.minY && py - PLAYER_RADIUS < box.maxY &&
            pz + PLAYER_RADIUS > box.minZ && pz - PLAYER_RADIUS < box.maxZ);
}

bool checkSphereCollision(float px, float py, float pz, const CollisionSphere& sphere) {
    float dx = px - sphere.x;
    float dz = pz - sphere.z;
    float distance = sqrt(dx*dx + dz*dz);
    return distance < (PLAYER_RADIUS + sphere.radius);
}

bool checkDoorTrigger(float px, float py, float pz, const BoundingBox& doorBox) {
    // Untuk pintu, kita perbolehkan player lebih dekat
    return (px > doorBox.minX && px < doorBox.maxX &&
            py > doorBox.minY && py < doorBox.maxY &&
            pz > doorBox.minZ && pz < doorBox.maxZ);
}

// Fungsi untuk memperbaiki posisi agar tidak tembus dinding
bool checkAndAdjustCollision(float &newX, float &newY, float &newZ, float oldX, float oldY, float oldZ) {
    bool collisionAdjusted = false;
    
    if (gameState == 0) { // DI LUAR
        // 1. CEGAH PERGI KE BELAKANG MUSEUM
        if (checkBoxCollision(newX, newY, newZ, noGoBehind)) {
            // Jika mencoba ke belakang museum, tolak gerakan
            newX = oldX;
            newZ = oldZ;
            collisionAdjusted = true;
        }
        
        // 2. Cek collision dengan gedung (kecuali pintu)
        if (checkBoxCollision(newX, newY, newZ, buildingBox)) {
            // Jika di area pintu, izinkan lewat
            if (!checkDoorTrigger(newX, newY, newZ, doorBoxOut)) {
                // Tolak gerakan mendekati gedung dari sisi lain
                newX = oldX;
                newZ = oldZ;
                collisionAdjusted = true;
            }
        }
        
        // 3. Cek collision dengan pilar
        for (const auto& pillar : pillars) {
            if (checkSphereCollision(newX, newY, newZ, pillar)) {
                // Hitung vektor dari pilar ke player
                float dx = newX - pillar.x;
                float dz = newZ - pillar.z;
                float dist = sqrt(dx*dx + dz*dz);
                float overlap = (PLAYER_RADIUS + pillar.radius) - dist;
                
                if (overlap > 0) {
                    // Dorong player keluar dari pilar
                    float pushX = (dx / dist) * overlap;
                    float pushZ = (dz / dist) * overlap;
                    newX += pushX;
                    newZ += pushZ;
                    collisionAdjusted = true;
                }
            }
        }
        
        // 4. Batasi area depan museum saja (tidak bisa ke samping terlalu jauh)
        if (newX < outsideBounds.minX || newX > outsideBounds.maxX ||
            newZ < outsideBounds.minZ || newZ > outsideBounds.maxZ) {
            newX = std::max(outsideBounds.minX, std::min(newX, outsideBounds.maxX));
            newZ = std::max(outsideBounds.minZ, std::min(newZ, outsideBounds.maxZ));
            collisionAdjusted = true;
        }
        
        // 5. Batasi ketinggian
        if (newY < outsideBounds.minY || newY > outsideBounds.maxY) {
            newY = std::max(outsideBounds.minY, std::min(newY, outsideBounds.maxY));
            collisionAdjusted = true;
        }
        
    } else if (gameState == 2) { // DI DALAM
        // 1. Cek dan perbaiki collision dengan dinding ruangan
        if (newX < insideRoom.minX) {
            newX = insideRoom.minX + COLLISION_EPSILON;
            collisionAdjusted = true;
        }
        if (newX > insideRoom.maxX) {
            newX = insideRoom.maxX - COLLISION_EPSILON;
            collisionAdjusted = true;
        }
        if (newZ < insideRoom.minZ) {
            newZ = insideRoom.minZ + COLLISION_EPSILON;
            collisionAdjusted = true;
        }
        if (newZ > insideRoom.maxZ) {
            // Kecuali di area pintu
            if (!(newX > -3 && newX < 3)) {
                newZ = insideRoom.maxZ - COLLISION_EPSILON;
                collisionAdjusted = true;
            }
        }
        
        // 2. Batasi ketinggian dalam ruangan
        if (newY < insideRoom.minY || newY > insideRoom.maxY) {
            newY = std::max(insideRoom.minY, std::min(newY, insideRoom.maxY));
            collisionAdjusted = true;
        }
        
        // 3. Cek collision dengan objek-objek dalam museum
        std::vector<BoundingBox> obstacles = {
            katanaBox, opikBox, rezaBox, andreBox, potBox, pictureBoxLeft, pictureBoxRight
        };
        
        for (const auto& obstacle : obstacles) {
            if (checkBoxCollision(newX, newY, newZ, obstacle)) {
                // Hitung push vector dari objek
                float centerX = (obstacle.minX + obstacle.maxX) / 2.0f;
                float centerZ = (obstacle.minZ + obstacle.maxZ) / 2.0f;
                
                // Dorong player keluar dari objek
                float dx = newX - centerX;
                float dz = newZ - centerZ;
                float dist = sqrt(dx*dx + dz*dz);
                
                if (dist > 0) {
                    float pushDistance = PLAYER_RADIUS + 
                        std::max(obstacle.maxX - obstacle.minX, obstacle.maxZ - obstacle.minZ) / 2.0f;
                    
                    newX = centerX + (dx / dist) * pushDistance;
                    newZ = centerZ + (dz / dist) * pushDistance;
                    collisionAdjusted = true;
                }
            }
        }
    }
    
    return collisionAdjusted;
}


// ========================================================================
// 4. FUNGSI PENCAHAYAAN (ORIGINAL)
// ========================================================================
void initLighting() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);

    GLfloat mat_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat mat_shininess[] = {50.0f};
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
}

void updateLightingLogic() {
    if (!lightingEnabled) { glDisable(GL_LIGHTING); return; }
    glEnable(GL_LIGHTING);

    GLfloat val_ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat val_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat val_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, val_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, val_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, val_specular);

    GLfloat light_pos[] = {0.0f, 20.0f, 0.0f, 1.0f};
    if (gameState == 0) {
        light_pos[0] = -30.0f; light_pos[1] = 50.0f; light_pos[2] = 50.0f;
    }
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
}

// ========================================================================
// 5. OBJEK INTERIOR (ORIGINAL)
// ========================================================================
void drawOpikArea(){ 
    glPushMatrix(); 
    glColor3f(0.2f, 0.2f, 0.2f); 
    glTranslatef(-10.0f, 1.0f, 10.0f); 
    glRotatef(45.0f, 0.0f, 1.0f, 0.0f); 
    glScalef(2.0f, 2.0f, 2.0f); 
    glutSolidCube(1.0f); 
    glPopMatrix(); }

void drawRezaArea(){ 
    glPushMatrix(); 
    glColor3f(0.2f, 0.2f, 0.2f); 
    glTranslatef(10.0f, 1.0f, 10.0f); 
    glRotatef(45.0f, 0.0f, 1.0f, 0.0f); 
    glScalef(2.0f, 2.0f, 2.0f); 
    glutSolidCube(1.0f); 
    glPopMatrix(); }

void drawAndreArea(){      
    glPushMatrix(); 
    glColor3f(0.2f, 0.2f, 0.2f); 
    glTranslatef(10.0f, 1.0f, -10.0f); 
    glRotatef(45.0f, 0.0f, 1.0f, 0.0f); 
    glScalef(2.0f, 2.0f, 2.0f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    
    GLUquadric* q = gluNewQuadric();

    
    glPushMatrix(); 
    glTranslatef(10.0f, 1.0f, -10.0f); 
    glRotatef(angle1, 0.0f, 1.0f, 0.0f); 
        
    glRotatef(5.0f, 0.0f, 1.0f, 0.0f); 
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); 
        glRotatef(12.0f, 1.0f, 0.0f, 0.0f);
        
        glColor3f(0.4f, 0.2f, 0.1f); 
        gluCylinder(q, 0.08f, 0.08f, 4.5f, 16, 4);
        
        glTranslatef(0.0f, 0.0f, 4.5f); 
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

void drawPot() {
    GLUquadric* q = gluNewQuadric();
    glPushMatrix(); 
    glTranslatef(-19.0f, 0.0f, 0.0f); 
    glColor3f(0.8f, 0.4f, 0.0f); 
    glScalef(0.5f, 0.5f, 0.5f);
        
    glPushMatrix(); 
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); 
        gluCylinder(q, 0.6f, 1.0f, 1.2f, 32, 8); 
        glPopMatrix();
        
        glPushMatrix(); 
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); 
        gluDisk(q, 0.0f, 0.6f, 32, 1); 
        glPopMatrix();
        
        glPushMatrix(); 
        glTranslatef(0.0f, 1.2f, 0.0f); 
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); 
        glutSolidTorus(0.08f, 1.0f, 16, 32); 
        glPopMatrix();
    
        glPopMatrix();
    
    glPushMatrix(); 
    glTranslatef(-19.0f, 0.0f, 0.0f); 
    glColor3f(0.0f, 0.6f, 0.0f);
        
    glPushMatrix(); 
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); 
        gluCylinder(q, 0.05f, 0.05f, 2.0f, 16, 4); 
        glPopMatrix();
        
        glTranslatef(0.0f, 2.0f, 0.0f); 
        glColor3f(1.0f, 1.0f, 0.0f); 
        glutSolidSphere(0.2f, 20, 20); 
        glColor3f(1.0f, 0.0f, 0.0f);
        for (int i = 0; i < 8; i++) { 
            glPushMatrix(); 
            glRotatef(i * 45.0f, 0.0f, 1.0f, 0.0f); 
            glTranslatef(0.4f, 0.0f, 0.0f); 
            glScalef(0.6f, 0.3f, 0.2f); 
            glutSolidSphere(0.4f, 16, 16); 
            glPopMatrix(); }
    
        glPopMatrix(); 
    gluDeleteQuadric(q);
}
void drawPictureFrame() {
    float width = 4.0f, height = 3.0f, thick = 0.2f;    
    glPushMatrix(); 
    glTranslatef(-7.0f, 5.0f, -19.9f); 
    glColor3f(0.5f, 0.3f, 0.1f); 
        
    glPushMatrix(); 
        glScalef(width, height, thick); 
        glutSolidCube(1.0f); 
        glPopMatrix();
        
        glPushMatrix(); 
        glColor3f(1.0f, 1.0f, 1.0f); 
        glTranslatef(0.0f, 0.0f, 0.11f); 
        glScalef(width * 0.75f, height * 0.75f, 0.01f); 
        glutSolidCube(1.0f); 
        glPopMatrix();
        
    glPopMatrix();
    
    glPushMatrix(); 
    glTranslatef(7.0f, 5.0f, -19.9f); 
    glColor3f(0.5f, 0.3f, 0.1f); 
        
    glPushMatrix(); 
        glScalef(width, height, thick); 
        glutSolidCube(1.0f); 
        glPopMatrix();
        
        glPushMatrix(); 
        glColor3f(1.0f, 1.0f, 1.0f); 
        glTranslatef(0.0f, 0.0f, 0.11f); 
        glScalef(width * 0.75f, height * 0.75f, 0.01f); 
        glutSolidCube(1.0f); 
        glPopMatrix();
    
        
        glPopMatrix();
}
void drawMuseumKatana() {
    glPushMatrix(); 
    glTranslatef(-10.0f, 0.0f, -10.0f); 
    glRotatef(45.0f, 0.0f, 1.0f, 0.0f); 
    
    glPushMatrix(); 
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    glDepthMask(GL_FALSE);
        
    glColor4f(0.6f, 0.9f, 1.0f, 0.25f); 
        glTranslatef(0.0f, 0.0f, 0.0f); 
        glScalef(7.0f, 10.0f, 6.7f); 
        glutSolidCube(1.0f);
    
        glDepthMask(GL_TRUE); 
    glDisable(GL_BLEND); 
    glPopMatrix();    
    
    glPushMatrix(); 
    glColor3f(0.0f, 0.0f, 0.0f); 
    glTranslatef(0.0f, 5.0f, 0.0f); 
    glScalef(7.0f, -0.2f, 7.0f); 
    glutSolidCube(1.0f); 
    glPopMatrix();    
    
    glPushMatrix(); 
    glColor3f(0.0f, 0.0f, 0.0f); 
    glTranslatef(0.0f, 0.0f, 0.0f); 
    glScalef(7.0f, 0.2f, 7.0f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    
    glPushMatrix(); 
    glColor3f(0.2f, 0.2f, 0.2f); 
    glTranslatef(0.0f, 1.0f, 0.0f); 
    glScalef(2.0f, 2.0f, 2.0f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    
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
    
    glPushMatrix(); 
    glTranslatef(0.0f, 2.5f, 0.0f); 
    glRotatef(5.0f, 0.0f, 0.0f, 1.0f); 
        
    glPushMatrix(); 
        glColor3f(0.9f, 0.9f, 0.95f); 
        glScalef(4.0f, 0.05f, 0.1f); 
        glutSolidCube(1.0f); 
        glPopMatrix();
        
        glPushMatrix(); 
        glColor3f(0.6f, 0.0f, 0.0f); 
        glTranslatef(2.3f, 0.0f, 0.0f); 
        glScalef(1.2f, 0.08f, 0.12f); 
        glutSolidCube(1.0f); 
        glPopMatrix();
        
        glPushMatrix(); 
        glColor3f(1.0f, 0.8f, 0.0f); 
        glTranslatef(1.8f, 0.0f, 0.0f); 
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f); 
        glutSolidTorus(0.02, 0.15, 10, 20); 
        glPopMatrix();
    
        glPopMatrix(); 
    glPopMatrix(); 
}
void drawLightSource() {
    if (!lightingEnabled) return;
    glDisable(GL_LIGHTING); 
    glPushMatrix(); 
    glTranslatef(0.0f, 5.0f, 0.0f);
    
    glColor3f(1.0f, 1.0f, 1.0f); 
    glutSolidSphere(0.15, 20, 20);
    
    glPopMatrix(); if (lightingEnabled) glEnable(GL_LIGHTING);
}

void drawSnakeLoading(const char* message) {
    glMatrixMode(GL_PROJECTION); 
    glPushMatrix(); 
    glLoadIdentity();
    
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    
    glMatrixMode(GL_MODELVIEW); 
    glPushMatrix(); 
    glLoadIdentity();
    
    glDisable(GL_LIGHTING); 
    glDisable(GL_DEPTH_TEST);

    // Background Hitam
    glBegin(GL_QUADS); 
    glColor3f(0.0f, 0.0f, 0.0f); 
    
    glVertex2f(0, 0); 
    glVertex2f(windowWidth, 0); 
    glVertex2f(windowWidth, windowHeight); 
    glVertex2f(0, windowHeight); 
    glEnd();

    // -- LOGIKA ULAR --
    float headX = windowWidth * loadingProgress;
    float centerY = windowHeight / 2.0f;
    float amplitude = 50.0f;
    
    glLineWidth(8.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < 50; i++) {
        float delay = i * 0.005f;
        float pos = loadingProgress - delay;
        
        if (pos > 0.0f) {
            float x = windowWidth * pos;
            float y = centerY + sin(pos * 20.0f) * amplitude;
            glColor3f(pos, 1.0f - pos, 0.2f);
            glVertex2f(x, y);
        }
    }
    glEnd();

    // Gambar Kepala Ular
    float headY = centerY + sin(loadingProgress * 20.0f) * amplitude;
    glPointSize(15.0f);
    glBegin(GL_POINTS);
        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex2f(headX, headY);
    glEnd();

    // Teks Loading
    glColor3f(1.0f, 1.0f, 1.0f); 
    glRasterPos2f(windowWidth/2 - 50, centerY - 100);
    for (const char* c = message; *c != '\0'; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    glEnable(GL_DEPTH_TEST); if (lightingEnabled) glEnable(GL_LIGHTING);
    glPopMatrix(); 
    glMatrixMode(GL_PROJECTION); 
    glPopMatrix(); 
    glMatrixMode(GL_MODELVIEW);
}

// 7. DRAW SCENES (EKSTERIOR GREEK & INTERIOR)
void drawDetailedPillar(float x, float z) {
    GLUquadric* q = gluNewQuadric();
    glPushMatrix(); 
    glTranslatef(x, 0.0f, z);
    
    glColor3f(0.7f, 0.7f, 0.7f); 
    glPushMatrix(); 
    glScalef(1.0f, 0.5f, 1.0f); 
    glutSolidTorus(0.2, 0.8, 10, 20); 
    glPopMatrix();
    
    glColor3f(0.9f, 0.9f, 0.95f); 
    glPushMatrix(); 
    glRotatef(-90, 1,0,0); 
    gluCylinder(q, 0.6f, 0.6f, 9.0f, 32, 2); 
    glPopMatrix();
    
    glColor3f(0.7f, 0.7f, 0.7f); 
    glTranslatef(0.0f, 9.0f, 0.0f); 
    glScalef(1.4f, 0.8f, 1.4f); 
    glutSolidCube(1.0f);
    
    glPopMatrix(); 
    gluDeleteQuadric(q);
}

void drawTree(float x, float z) {
    glPushMatrix(); 
    glTranslatef(x, 0.0f, z);
    
    glColor3f(0.4f, 0.2f, 0.1f); 
    glPushMatrix(); 
    glRotatef(-90, 1,0,0); 
    glutSolidCone(0.5, 4.0, 10, 10); 
    glPopMatrix();
    
    glColor3f(0.0f, 0.5f, 0.1f); 
    glTranslatef(0.0f, 1.5f, 0.0f);
    glPushMatrix(); 
    glRotatef(-90, 1,0,0); 
    glutSolidCone(2.0, 4.0, 10, 10); 
    glPopMatrix();
    
    glTranslatef(0.0f, 1.5f, 0.0f); 
    glPushMatrix(); 
    glRotatef(-90, 1,0,0); 
    glutSolidCone(1.5, 3.5, 10, 10); 
    glPopMatrix();
    
    glPopMatrix();
}

void drawExteriorScene() {
    // Tanah & Jalan
    glDisable(GL_LIGHTING);
    glColor3f(0.1f, 0.35f, 0.1f); 
    glBegin(GL_QUADS); 
    glVertex3f(-100,0,-100); 
    glVertex3f(-100,0,100); 
    glVertex3f(100,0,100); 
    glVertex3f(100,0,-100); 
    glEnd();

    glColor3ub(139, 69, 19); 

    glBegin(GL_QUADS); 
    glVertex3f(-8,0.01,10); 
    glVertex3f(-8,0.01,60); 
    glVertex3f(8,0.01,60); 
    glVertex3f(8,0.01,10); 
    glEnd();

    glEnable(GL_LIGHTING);
    // Tangga & Teras
    glColor3f(0.6f, 0.6f, 0.65f);
    for(int i=0; i<8; i++) { 
        glPushMatrix(); 
        glTranslatef(0.0f, i * 0.2f, 16.0f - i * 0.4f); 
        glScalef(26.0f, 0.2f, 0.4f); 
        glutSolidCube(1.0f); 
        glPopMatrix(); 
    }
        glPushMatrix(); 
        glTranslatef(0.0f, 1.6f, 10.0f); 
        glScalef(26.0f, 0.2f, 6.0f); 
        glutSolidCube(1.0f); 
        glPopMatrix();

    // Pilar
    for(int i=0; i<6; i++) drawDetailedPillar(-10.0f + i*4.0f, 10.0f);
    glPushMatrix();
    glColor3f(0.75f, 0.75f, 0.75f);
    glTranslatef(0.0f, 10.0f, 10.2f);
    glScalef(30.0f, 1.0f, 1.0f);          
    glutSolidCube(1.0f);
    glPopMatrix();

    // Gedung
    glPushMatrix(); 
    glColor3f(0.85f, 0.82f, 0.78f); 
    glTranslatef(0.0f, 6.0f, -5.0f); 
    glScalef(32.0f, 12.0f, 30.0f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    
    glBegin(GL_TRIANGLES);
    glColor3f(0.8f, 0.8f, 0.8f);
    glVertex3f(-12.0f, 9.5f, 10.3f);
    glVertex3f(12.0f, 9.5f, 10.3f);
    glVertex3f(0.0f, 12.0f, 10.5f);
    glEnd();

    // Atap
    glPushMatrix(); 
    glTranslatef(0.0f, 12.0f, 10.0f); 
    glBegin(GL_TRIANGLES); 
    glColor3f(0.6f, 0.6f, 0.65f); 
    glNormal3f(0,0,1); 
    glVertex3f(-16,0,0); 
    glVertex3f(16,0,0); 
    glVertex3f(0,6,0); 
    glEnd();
    
    glBegin(GL_QUADS); 
    glColor3f(0.5f, 0.2f, 0.1f); 
    glNormal3f(-1,1,0); 
    glVertex3f(-16,0,0);
    glVertex3f(0,6,0); 
    glVertex3f(0,6,-20); 
    glVertex3f(-16,0,-20);
    
    glNormal3f(1,1,0); 
    glVertex3f(16,0,0); 
    glVertex3f(0,6,0); 
    glVertex3f(0,6,-20); 
    glVertex3f(16,0,-20); 
    glEnd();
    
    glPopMatrix();
    // Pintu
    glPushMatrix(); 
    glColor3f(0.2f, 0.1f, 0.0f); 
    glTranslatef(0.0f, 4.0f, 10.1f); 
    glScalef(6.0f, 8.0f, 0.2f); 
    glutSolidCube(1.0f); 
    glPopMatrix();

    // Pohon
    drawTree(-15, 25); 
    drawTree(15, 25); 
    drawTree(-22, 35); 
    drawTree(22, 35);
    
    // Area Dilarang (belakang museum) - visual indicator
    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 0.0f, 0.0f, 0.3f);
    for(int x = -15; x <= 15; x += 2) {
        for(int z = -30; z <= 8; z += 2) {
            glBegin(GL_QUADS);
            glVertex3f(x, 0.1f, z);
            glVertex3f(x+1, 0.1f, z);
            glVertex3f(x+1, 0.1f, z+1);
            glVertex3f(x, 0.1f, z+1);
            glEnd();
        }
    }
    glEnable(GL_LIGHTING);
}

void drawInteriorSceneFull() {
    // Ruangan Box Tertutup
    glDisable(GL_CULL_FACE);
    // Lantai
    glBegin(GL_QUADS); 
    glColor3ub(255, 255, 255); 
    glVertex3f(-27,0,-27); 
    glVertex3f(-27,0,27); 
    glVertex3f(27,0,27); 
    glVertex3f(27,0,-27); 
    glEnd();
    // Dinding (DIPERBAIKI agar lebih tebal)
    float s = 20.0f, h = 9.0f; 
    glColor3ub(92, 51, 23);
    
    // Dinding dengan ketebalan
    for (float offset = 0; offset <= 0.3f; offset += 0.1f) {
        glBegin(GL_QUADS); 
        // Kiri
        glVertex3f(-s-offset,0,-s); 
        glVertex3f(-s-offset,0,s); 
        
        glVertex3f(-s-offset,h,s); 
        glVertex3f(-s-offset,h,-s);
        // Kanan
        glVertex3f(s+offset,0,-s); 
        glVertex3f(s+offset,0,s); 
        
        glVertex3f(s+offset,h,s); 
        glVertex3f(s+offset,h,-s);
        // Belakang
        glVertex3f(-s,0,-s-offset); 
        glVertex3f(s,0,-s-offset); 
        
        glVertex3f(s,h,-s-offset); 
        glVertex3f(-s,h,-s-offset);
        // Depan (kecuali pintu)
        glVertex3f(-s,0,s+offset); 
        glVertex3f(-5,0,s+offset); 
        
        glVertex3f(-5,h,s+offset); 
        glVertex3f(-s,h,s+offset);
        
        glVertex3f(5,0,s+offset); 
        glVertex3f(s,0,s+offset); 
        
        glVertex3f(s,h,s+offset); 
        glVertex3f(5,h,s+offset);
        
        glVertex3f(-5,7,s+offset); 
        glVertex3f(5,7,s+offset); 
        
        glVertex3f(5,h,s+offset); 
        glVertex3f(-5,h,s+offset);
        
        glEnd();
    }
    
    // Atap
    glBegin(GL_QUADS); 
    glColor3ub(101, 67, 33); 
    glNormal3f(0,-1,0); 
    
    glVertex3f(-27,9,-27); 
    glVertex3f(-27,9,27); 
    glVertex3f(27,9,27); 
    glVertex3f(27,9,-27); 
    glEnd();
    
    // Pintu Keluar
    glPushMatrix(); 
    glColor3f(0.1f, 0.0f, 0.0f); 
    glTranslatef(0.0f, 3.5f, 19.8f); 
    glScalef(6.0f, 7.0f, 0.4f); 
    glutSolidCube(1.0f); 
    glPopMatrix();
    
    glEnable(GL_CULL_FACE);

    drawRezaArea(); 
    drawOpikArea(); 
    drawMuseumKatana(); 
    drawPot(); 
    drawPictureFrame(); 
    drawAndreArea(); 
    drawLightSource();
}

// 8. LOGIKA KAMERA & DISPLAY UTAMA DENGAN COLLISION YANG DIPERBAIKI
void updateCamera() {
    // --- LOADING ANIMATION ---
    if (gameState == 1 || gameState == 3) {
        loadingProgress += 0.002f;
        
        if (loadingProgress >= 1.0f) {
            loadingProgress = 0.0f;
            if (gameState == 1) {
                gameState = 2;
                cameraPosX = 0.0f; cameraPosY = 3.0f; cameraPosZ = 8.0f; 
                cameraYaw = 0.0f; cameraPitch = -20.0f;
            } 
            else if (gameState == 3) {
                gameState = 0;
                cameraPosX = 0.0f; cameraPosY = 3.5f; cameraPosZ = 45.0f; 
                cameraYaw = -90.0f; cameraPitch = 0.0f;
            }
        }
        return;
    }

    // --- PLAYER MOVEMENT DENGAN COLLISION YANG DIPERBAIKI ---
    float yawRad = cameraYaw * PI / 180.0f;
    float pitchRad = cameraPitch * PI / 180.0f;
    
    float forwardX = sin(yawRad) * cos(pitchRad);
    float forwardY = -sin(pitchRad);
    float forwardZ = -cos(yawRad) * cos(pitchRad);
    
    float rightX = sin(yawRad + PI/2);
    float rightZ = -cos(yawRad + PI/2);

    // Simpan posisi awal untuk collision recovery
    float oldX = cameraPosX;
    float oldY = cameraPosY;
    float oldZ = cameraPosZ;
    
    // Hitung posisi baru berdasarkan input
    float newX = cameraPosX;
    float newY = cameraPosY;
    float newZ = cameraPosZ;

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
    if (keys[' ']) {
        newY += MOVE_SPEED;
    }
    if (keys['c'] || keys['C']) {
        newY -= MOVE_SPEED;
    }

    // Cek dan perbaiki collision dengan sistem baru
    checkAndAdjustCollision(newX, newY, newZ, oldX, oldY, oldZ);

    // Update posisi kamera
    cameraPosX = newX;
    cameraPosY = newY;
    cameraPosZ = newZ;

    // TRIGGER ZONES untuk pintu
    if (gameState == 0) { // Di Luar
        if (cameraPosZ < 14.0f && cameraPosZ > 8.0f && fabs(cameraPosX) < 4.0f) {
            gameState = 1;
        }
    } 
    else if (gameState == 2) { // Di Dalam
        if (cameraPosZ > 18.0f && fabs(cameraPosX) < 5.0f) {
            gameState = 3;
        }
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (gameState == 1) { 
        drawSnakeLoading("ENTERING MUSEUM..."); 
        glutSwapBuffers(); 
        return; 
    } 
    if (gameState == 3) { 
        drawSnakeLoading("LEAVING MUSEUM..."); 
        glutSwapBuffers(); 
        return; 
    }

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0, (double)windowWidth / windowHeight, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    // Set Kamera
    float yawRad = cameraYaw * PI / 180.0f;
    float pitchRad = cameraPitch * PI / 180.0f;
    float lookX = cameraPosX + sin(yawRad) * cos(pitchRad);
    float lookY = cameraPosY - sin(pitchRad);
    float lookZ = cameraPosZ - cos(yawRad) * cos(pitchRad);
    
    gluLookAt(cameraPosX, cameraPosY, cameraPosZ, lookX, lookY, lookZ, 0.0f, 1.0f, 0.0f);

    angle1 += 1.1f; if (angle1 > 2.0f) angle1 -= 360.0f;
    updateLightingLogic();

    if (gameState == 0) {
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); 
        drawExteriorScene();
    } else if (gameState == 2) {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f); 
        drawInteriorSceneFull();
    }

    // Tampilkan status collision (debug)
    glMatrixMode(GL_PROJECTION); 
    glPushMatrix(); 
    glLoadIdentity();
    
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    
    glMatrixMode(GL_MODELVIEW); 
    glPushMatrix(); 
    glLoadIdentity();
    
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(20, 40);
    
    std::string status;
    if (gameState == 0) {
        status = "AREA: Outside Museum | CANNOT go behind building";
    } else {
        status = "AREA: Inside Museum | Collision: ACTIVE";
    }
    for (const char* c = status.c_str(); *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    // Tampilkan posisi kamera
    glRasterPos2f(20, 60);
    std::string posText = "Pos: X=" + std::to_string((int)cameraPosX) + 
                         " Y=" + std::to_string((int)cameraPosY) + 
                         " Z=" + std::to_string((int)cameraPosZ);
    for (const char* c = posText.c_str(); *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    glEnable(GL_LIGHTING);
    glPopMatrix(); 
    glMatrixMode(GL_PROJECTION); 
    glPopMatrix(); 
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    keys[key] = true;
    if (key == 27) exit(0);
}
void keyboardUp(unsigned char key, int x, int y) { keys[key] = false; }
void specialKey(int key, int x, int y) { if (key == GLUT_KEY_CTRL_L || key == GLUT_KEY_CTRL_R) keys[key] = true; }
void specialKeyUp(int key, int x, int y) { if (key == GLUT_KEY_CTRL_L || key == GLUT_KEY_CTRL_R) keys[key] = false; }

void mouseMotion(int x, int y) {
    int centerX = windowWidth / 2;
    int centerY = windowHeight / 2;
    if (x == centerX && y == centerY) return;
    
    float deltaX = (x - centerX) * MOUSE_SENSITIVITY * 50.0f;
    float deltaY = (y - centerY) * MOUSE_SENSITIVITY * 50.0f;
    
    cameraYaw += deltaX;
    cameraPitch += deltaY;
    
    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;
    
    glutWarpPointer(centerX, centerY);
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        mouseActive = true; 
        glutSetCursor(GLUT_CURSOR_NONE); 
        glutWarpPointer(windowWidth/2, windowHeight/2);
    }
}

void idle() { 
    updateCamera(); 
    glutPostRedisplay(); 
}

void reshape(int w, int h) { 
    windowWidth = w; 
    windowHeight = h; 
    glViewport(0, 0, w, h); 
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Museum - No Wall Penetration & Restricted Area");
    glutFullScreen();
    glutSetCursor(GLUT_CURSOR_NONE);
    
    initLighting();
    initPillars();
    
    glutDisplayFunc(display); 
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard); 
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKey); 
    glutSpecialUpFunc(specialKeyUp);
    glutPassiveMotionFunc(mouseMotion); 
    glutMotionFunc(mouseMotion); 
    glutMouseFunc(mouse);
    glutIdleFunc(idle);
    
    glutMainLoop();
    return 0;
}