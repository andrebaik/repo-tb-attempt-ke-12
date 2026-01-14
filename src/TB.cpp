#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
//1
// KONSTANTA 
const float PI = 3.14159265f;
const float MOVE_SPEED = 0.25f; 
const float MOUSE_SENSITIVITY = 0.002f;

// VARIABEL KAMERA 
float cameraPosX = 0.0f, cameraPosY = 6.0f, cameraPosZ = 25.0f; 
float cameraYaw = 0.0f, cameraPitch = -10.0f; 

// VARIABEL INPUT 
int lastMouseX = 0, lastMouseY = 0;
bool mouseActive = false;
bool keys[256] = {false};
int windowWidth = 1024, windowHeight = 768;

// LIGHTING & ENVIRONMENT 
bool lightingEnabled = true;
int lightMode = 1;      // 1: Siang, 2: Malam
bool isSun = true;      
bool lampOn = true;     
float waterAnim = 0.0f; 

// Posisi Matahari/Bulan
float sunDir[3] = {20.0f, 60.0f, -35.0f};     
float moonDir[3] = {-20.0f, 60.0f, -35.0f};   

// VARIABEL OBJEK 
float gazeboX = 0.0f;        
float gazeboZ = -18.0f;       
float gazeboScale = 1.0f;    

// Kucing 
float catX = 10.0f;          
float catZ = 0.0f;          
float catRot = 90.0f;        
float catScale = 1.0f;      

// UKURAN KOLAM & JALAN 
const float POND_RADIUS = 7.0f;
const float PATH_INNER_R = 7.2f;
const float PATH_OUTER_R = 11.5f;

// STRUKTUR POHON 
struct Tree {
    float x, z;
    float height;
    float trunkWidth;
};
std::vector<Tree> trees;

// WARNA 
GLfloat woodColor[] = {0.5f, 0.35f, 0.15f, 1.0f};
GLfloat leafColor[] = {0.1f, 0.6f, 0.1f, 1.0f};
//1
// tidak menaruh pohon satu per satu,  pakai 'Procedural Generation
// Ada logic 'Collision Detection' (Cek Jarak) biar pohon tidak tumbuh di air atau nabrak gazebo.
void initTrees() {
    srand(time(0));
    trees.clear(); 

    int maxTrees = 80;          
    float minDistance = 3.5f; 

    for (int i = 0; i < maxTrees; i++) {
        Tree tree;
        bool valid = false;
        int attempts = 0;
        // Logic Loop untuk mencari posisi aman
        while (!valid && attempts < 1000) {
            attempts++;
            tree.x = -35.0f + static_cast<float>(rand()) / RAND_MAX * 70.0f;
            tree.z = -35.0f + static_cast<float>(rand()) / RAND_MAX * 70.0f;
            
            float distCenter = sqrt(tree.x*tree.x + tree.z*tree.z);
            
            // Cek Tabrakan dengan Jalan Melingkar & Kolam
            bool onPathOrWater = (distCenter < PATH_OUTER_R + 1.0f); 

            // Cek Tabrakan dengan Gazebo
            float distGazebo = sqrt(pow(tree.x - gazeboX, 2) + pow(tree.z - gazeboZ, 2));
            bool nearGazebo = (distGazebo < 6.0f);

            bool tooCloseToOtherTree = false;
            for (const auto& other : trees) {
                float dx = tree.x - other.x;
                float dz = tree.z - other.z;
                float dist = sqrt(dx*dx + dz*dz);
                if (dist < minDistance) {
                    tooCloseToOtherTree = true;
                    break; 
                }
            }
            
            if (!onPathOrWater && !nearGazebo && !tooCloseToOtherTree) {                      
                valid = true;
            }
        }
        
        if (valid) {
            tree.height = 4.0f + static_cast<float>(rand()) / RAND_MAX * 3.0f;
            tree.trunkWidth = 0.35f;
            trees.push_back(tree);
        }
    }
}

// GAMBAR MATAHARI/BULAN
void drawSunMoon() {
    glPushMatrix();
    if (isSun) {
        glTranslatef(sunDir[0], sunDir[1], sunDir[2]);
    } else {
        glTranslatef(moonDir[0], moonDir[1], moonDir[2]);
    }

    glDisable(GL_LIGHTING); // Matikan lighting agar matahari tetap terang sendiri

    if (isSun) {
        glColor3f(1.0, 1.0, 0.6); 
        glutSolidSphere(3.0, 40, 40); 
        
        
        //Menggunakan GL_BLEND (Alpha Blending) untuk efek cahaya berpendar (Glow)."
        glEnable(GL_BLEND);
        glColor4f(1.0f, 1.0f, 0.6f, 0.2f);
        glutSolidSphere(6.0, 30, 30); 
        glDisable(GL_BLEND);

    } else {
        glColor3f(0.8, 0.8, 1.0); 
        glutSolidSphere(3.0, 40, 40);
    }

    glEnable(GL_LIGHTING);
    glPopMatrix();
}


// lampu jalan memiliki fitur 'Emissive Material' (Lampu menyala sendiri saat malam)."
void drawStreetLamp(float x, float z, float rotY) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(rotY, 0, 1, 0); 
    
    // Dudukan
    glColor3f(0.2f, 0.2f, 0.2f);
    glPushMatrix();
    glTranslatef(0.0f, 0.1f, 0.0f);
    glScalef(0.6f, 0.2f, 0.6f); 
    glutSolidCube(1.0f);
    glPopMatrix();

    // Tiang
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 1.5f, 0.0f); 
    glScalef(0.15f, 3.0f, 0.15f);   
    glutSolidCube(1.0f);
    glPopMatrix();

    // Lengan Lampu
    glPushMatrix();
    glTranslatef(0.0f, 2.8f, 0.3f); 
    glRotatef(-20, 1, 0, 0);
    glScalef(0.1f, 0.1f, 0.8f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Kepala Lampu (Lampion)
    glPushMatrix();
    glTranslatef(0.0f, 2.6f, 0.7f); 

//    Logika Emisi Cahaya (Menyala/Mati)
    if (lampOn) { 
        GLfloat emission[] = { 1.5f, 1.4f, 0.5f, 1.0f }; 
        glMaterialfv(GL_FRONT, GL_EMISSION, emission); // Bikin objek bersinar
        glColor3f(1.0f, 1.0f, 0.6f);
    } else { 
        GLfloat no_emission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, no_emission);
        glColor3f(0.8f, 0.8f, 0.8f); 
    }
    glutSolidSphere(0.3f, 12, 12); 

    // Reset emisi ke 0 agar objek lain tidak ikut bersinar
    GLfloat default_emission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_EMISSION, default_emission);
    glPopMatrix();

    glPopMatrix(); 
}
// 2.

//OBJEK JALAN]
// jalan melingkar menggunakan matematika Trigonometri (Sin/Cos) dan GL_QUAD_STRIP agar mulus."
void drawCircularPath() {
    GLfloat stoneDiff[] = {0.7f, 0.65f, 0.6f, 1.0f}; 
    glMaterialfv(GL_FRONT, GL_DIFFUSE, stoneDiff);
    
    float y = 0.05f; 
    int segments = 100;

    glBegin(GL_QUAD_STRIP);
    for(int i = 0; i <= segments; i++) {
        float angle = 2.0f * PI * (float)i / (float)segments;
        float c = cos(angle);
        float s = sin(angle);
        
        if (i % 2 == 0) glColor3f(0.65f, 0.6f, 0.55f);
        else glColor3f(0.7f, 0.65f, 0.6f);

        glNormal3f(0, 1, 0);
        // Koordinat jalan dihitung pakai Sin/Cos
        glVertex3f(c * PATH_INNER_R, y, s * PATH_INNER_R); // Titik Dalam
        glVertex3f(c * PATH_OUTER_R, y, s * PATH_OUTER_R); // Titik Luar
    }
    glEnd();

}


//alang ini bergerak ditiup angin menggunakan fungsi Sinus(waktu)."
void drawReeds(float x, float z) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    
    glColor3f(0.2f, 0.5f, 0.1f);
    for(int i=0; i<3; i++) {
        glPushMatrix();
        glTranslatef((i*0.15f)-0.15f, 0.0f, (i%2 * 0.1f));
        
        //  Rotasi dinamis berdasarkan waktu (waterAnim)
        glRotatef((sin(waterAnim + x) * 5.0f) + (i*10), 1, 0, 1);
        
        // Gambar batang & kepala...
        glPushMatrix(); 
        glScalef(0.05f, 1.5f, 0.05f);
        glTranslatef(0, 0.5f, 0);
        glutSolidCube(1.0f);
        glPopMatrix();

        glColor3f(0.4f, 0.2f, 0.1f);
        glPushMatrix();
        glTranslatef(0.0f, 1.5f, 0.0f);
        glScalef(0.08f, 0.4f, 0.08f);
        glutSolidSphere(1.0f, 8, 8);
        glPopMatrix();
        
        glColor3f(0.2f, 0.5f, 0.1f); 
        glPopMatrix();
    }
    glPopMatrix();
}


// Objek ini mengapung naik-turun mengikuti gelombang air (Logic Y + Sin)."
void drawLotus(float x, float z, float size, float rotation) {
    glPushMatrix();
    // Logic mengapung
    glTranslatef(x, 0.05f + sin(waterAnim + x)*0.02f, z); 
    glRotatef(rotation + waterAnim * 5, 0, 1, 0);
    glScalef(size, size, size);

    // Daun & Kelopak...
    glColor3f(0.1f, 0.6f, 0.2f);
    glPushMatrix();
    glScalef(1.0f, 0.02f, 1.0f);
    glutSolidSphere(0.8f, 16, 8);
    glPopMatrix();
    
   
    glColor3f(1.0f, 0.6f, 0.8f);
    for(int i=0; i<8; i++) {
        glPushMatrix();
        glRotatef(i * 45, 0, 1, 0);
        glTranslatef(0.3f, 0.1f, 0.0f);
        glRotatef(-30, 0, 0, 1);
        glScalef(0.4f, 0.1f, 0.2f);
        glutSolidSphere(1.0f, 8, 8);
        glPopMatrix();
    }
    
    // Pusat kuning
    glColor3f(1.0f, 1.0f, 0.0f);
    glPushMatrix(); glTranslatef(0.0f, 0.15f, 0.0f); glutSolidSphere(0.15f, 8, 8); glPopMatrix();

    glPopMatrix();
}

void drawTree(float x, float z, float height, float trunkWidth) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glColor3fv(woodColor);
    // menggambar batang dan daun cemara
    glPushMatrix(); glScalef(trunkWidth, height * 0.6f, trunkWidth); glTranslatef(0.0f, 0.5f, 0.0f); glutSolidCube(1.0f); glPopMatrix();
    glColor3fv(leafColor);
    for(int i=0; i<3; i++) {
        glPushMatrix(); glTranslatef(0.0f, height * 0.5f + (i * height * 0.25f), 0.0f); glRotatef(-90, 1, 0, 0); glutSolidCone(height * 0.45f - (i*0.25f), height * 0.5f, 10, 2); glPopMatrix();
    }
    glPopMatrix();
}

void drawGardenBench(float x, float z, float rot) {
    //kode bangku taman, penjelasannya ada di bagian Loop di fungsi Display
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(rot, 0.0f, 1.0f, 0.0f);
    glColor3f(0.2f, 0.2f, 0.2f); // Kaki
    glPushMatrix(); glTranslatef(-0.6f, 0.2f, 0.0f); glScalef(0.1f, 0.4f, 0.4f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.6f, 0.2f, 0.0f); glScalef(0.1f, 0.4f, 0.4f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.6f, 0.4f, 0.2f); // Dudukan
    glPushMatrix(); glTranslatef(0.0f, 0.45f, 0.0f); glScalef(1.5f, 0.1f, 0.5f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.85f, -0.2f); glScalef(1.5f, 0.4f, 0.1f); glutSolidCube(1.0f); glPopMatrix();
    glPopMatrix();
}

// Modeling. Kepala, kaki, dan ekor menempel pada badan. Transformasi satu bagian mempengaruhi anaknya."
void drawCat() {
    glPushMatrix();
    glTranslatef(catX, 0.25f, catZ); 
    glRotatef(catRot, 0.0f, 1.0f, 0.0f);
    glScalef(catScale, catScale, catScale);

    // Badan
    glColor3f(1.0f, 0.6f, 0.2f); 
    glPushMatrix(); glTranslatef(0.0f, 0.5f, 0.0f); glScalef(0.6f, 0.5f, 1.2f); glutSolidCube(1.0f); glPopMatrix();

    // Kepala (Sphere)
    glPushMatrix(); glTranslatef(0.0f, 0.8f, 0.7f); glutSolidSphere(0.35f, 20, 20);
    // mata, hidung, telinga ada di sini
    glPopMatrix();

    // Kaki kucing ada di sini

    // Ekor Bergerak
    glPushMatrix();
    glTranslatef(0.0f, 0.6f, -0.6f);
    glRotatef(45 + sin(waterAnim)*20, 0, 1, 0); // Logic goyang ekor
    glRotatef(45, 1, 0, 0);
    glScalef(0.1f, 0.6f, 0.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPopMatrix();
}

//OBJEK GAZEBO
//Struktur kompleks dari bentuk dasar. Tiang = Cube dipanjangin, Atap = Cone dirotasi."
void drawGazebo() {
    glPushMatrix();
    glTranslatef(gazeboX, 0.2f, gazeboZ); 
    glScalef(gazeboScale, gazeboScale, gazeboScale); 

    // Lantai
    glColor3f(0.5f, 0.3f, 0.1f);
    glPushMatrix(); glTranslatef(0.0f, 0.1f, 0.0f); glScalef(4.0f, 0.2f, 4.0f); glutSolidCube(1.0f); glPopMatrix();

    // Tiang
    glColor3f(0.6f, 0.4f, 0.2f);
    for (int i = -1; i <= 1; i += 2) {
        for (int j = -1; j <= 1; j += 2) {
            glPushMatrix();
            glTranslatef(i * 1.5f, 1.2f, j * 1.5f); 
            glScalef(0.2f, 2.2f, 0.2f); 
            glutSolidCube(1.0f);
            glPopMatrix();
        }
    }
    // Atap
    glColor3f(0.8f, 0.2f, 0.1f); 
    glPushMatrix();
    glTranslatef(0.0f, 2.3f, 0.0f);
    glRotatef(-90, 1.0f, 0.0f, 0.0f);
    glRotatef(45, 0.0f, 0.0f, 1.0f); 
    glutSolidCone(3.0f, 1.5f, 4, 1); 
    glPopMatrix();
    glPopMatrix();
}

void drawEnvironment() {
    // Rumput...
    GLfloat grassAmb[] = {0.2f, 0.5f, 0.2f, 1.0f};
    GLfloat grassDiff[] = {0.2f, 0.7f, 0.2f, 1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT, grassAmb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, grassDiff);
    glColor3f(0.2f, 0.6f, 0.2f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-100, 0, -100); glVertex3f(-100, 0, 100); glVertex3f(100, 0, 100); glVertex3f(100, 0, -100);
    glEnd();

    //  EFEK AIR
    // Air dibuat menggunakan Triangle Fan dengan logic Sinus untuk efek ombak."
    glPushMatrix();
    glTranslatef(0.0f, 0.02f, 0.0f); 
    
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glColor4f(0.0f, 0.3f, 0.8f, 0.9f); // Pusat
    glVertex3f(0, 0, 0); 
    
    glColor4f(0.0f, 0.5f, 0.9f, 0.8f); // Pinggir
    int segments = 64;
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * PI * (float)i / (float)segments;
        // Logic Ombak
        float wave = sin(angle * 6.0f + waterAnim) * 0.05f; 
        glVertex3f(cos(angle) * (POND_RADIUS + wave), 0.0f, sin(angle) * (POND_RADIUS + wave));
    }
    glEnd();
    glPopMatrix();
}

void initLighting() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0); 
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);
}

void setupPointLight(int lightID, float x, float z) {
    if (lampOn) {
        glEnable(lightID);
        GLfloat lightColor[] = { 1.0f, 0.9f, 0.6f, 1.0f }; 
        GLfloat lightPos[] = { x, 3.5f, z, 1.0f }; 
        glLightfv(lightID, GL_DIFFUSE, lightColor);
        glLightfv(lightID, GL_POSITION, lightPos);
        glLightf(lightID, GL_CONSTANT_ATTENUATION, 0.5f);
        glLightf(lightID, GL_LINEAR_ATTENUATION, 0.1f);
        glLightf(lightID, GL_QUADRATIC_ATTENUATION, 0.02f);
    } else {
        glDisable(lightID);
    }
}

//  LOGIKA SIANG/MALAM
// Fungsi utama yang mengatur nuansa scene. Mengubah warna Ambient Light & Background Color."
void updateLightingLogic() {
    if (!lightingEnabled) {
        glDisable(GL_LIGHTING);
        return;
    }
    glEnable(GL_LIGHTING);

    if (lightMode == 1) { // SIANG
        GLfloat light_pos[] = {sunDir[0], sunDir[1], sunDir[2], 0.0f}; 
        glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
        GLfloat diffuse[] = {0.9f, 0.9f, 0.9f, 1.0f}; 
        GLfloat ambient[] = {0.4f, 0.4f, 0.4f, 1.0f};   
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
        glClearColor(0.5f, 0.8f, 0.95f, 1.0f); // Langit Biru

    } else { // MALAM
        GLfloat light_pos[] = {moonDir[0], moonDir[1], moonDir[2], 0.0f}; 
        glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
        GLfloat diffuse[] = {0.05f, 0.05f, 0.15f, 1.0f}; // Cahaya redup
        GLfloat ambient[] = {0.05f, 0.05f, 0.1f, 1.0f};  // Ambient gelap
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
        glClearColor(0.02f, 0.02f, 0.05f, 1.0f); // Langit Gelap
    }

    float lampR = PATH_OUTER_R + 0.5f;
    setupPointLight(GL_LIGHT1, lampR, 0);            
    setupPointLight(GL_LIGHT2, -lampR, 0);           
    setupPointLight(GL_LIGHT3, 0, lampR);            
    setupPointLight(GL_LIGHT4, 0, -lampR);           
}

void updateCamera() {
    //Logika kamera standar menggunakan Sin/Cos untuk forward vector
    float yawRad = cameraYaw * PI / 180.0f;
    float pitchRad = cameraPitch * PI / 180.0f;
    float forwardX = sin(yawRad) * cos(pitchRad);
    float forwardY = -sin(pitchRad);
    float forwardZ = -cos(yawRad) * cos(pitchRad);
    float rightX = sin(yawRad + PI/2);
    float rightZ = -cos(yawRad + PI/2);

    if (keys['w'] || keys['W']) { cameraPosX += forwardX * MOVE_SPEED; cameraPosY += forwardY * MOVE_SPEED; cameraPosZ += forwardZ * MOVE_SPEED; }
    if (keys['s'] || keys['S']) { cameraPosX -= forwardX * MOVE_SPEED; cameraPosY -= forwardY * MOVE_SPEED; cameraPosZ -= forwardZ * MOVE_SPEED; }
    if (keys['a'] || keys['A']) { cameraPosX -= rightX * MOVE_SPEED; cameraPosZ -= rightZ * MOVE_SPEED; }
    if (keys['d'] || keys['D']) { cameraPosX += rightX * MOVE_SPEED; cameraPosZ += rightZ * MOVE_SPEED; }
    if (keys[' ']) cameraPosY += MOVE_SPEED;
    if (keys['z'] || keys['Z']) cameraPosY -= MOVE_SPEED;
    if (cameraPosY < 1.0f) cameraPosY = 1.0f;

    // Kontrol Kucing
    if (keys['i'] || keys['I']) catZ -= 0.1f; 
    if (keys['k'] || keys['K']) catZ += 0.1f; 
    if (keys['j'] || keys['J']) catX -= 0.1f; 
    if (keys['l'] || keys['L']) catX += 0.1f; 
    if (keys['o'] || keys['O']) catScale += 0.01f; 
    if (keys['p'] || keys['P']) if (catScale > 0.1f) catScale -= 0.01f;
    if (keys['r'] || keys['R']) catRot += 2.0f; 
    if (keys['e'] || keys['E']) catRot -= 2.0f; 
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    float yawRad = cameraYaw * PI / 180.0f;
    float pitchRad = cameraPitch * PI / 180.0f;
    float lookX = cameraPosX + sin(yawRad) * cos(pitchRad);
    float lookY = cameraPosY - sin(pitchRad);
    float lookZ = cameraPosZ - cos(yawRad) * cos(pitchRad);
    gluLookAt(cameraPosX, cameraPosY, cameraPosZ, lookX, lookY, lookZ, 0.0f, 1.0f, 0.0f);

    updateLightingLogic(); 

    drawSunMoon();
    drawEnvironment();
    drawCircularPath(); 
    
    // Objek Hiasan
    drawLotus(3.0f, 2.0f, 1.0f, 0);
    drawLotus(-2.5f, -3.0f, 0.8f, 45);
    drawLotus(4.0f, -1.0f, 1.2f, 90);
    drawLotus(-1.0f, 4.5f, 0.9f, 135);

    for(int i=0; i<8; i++) {
        float ang = i * (360.0f/8.0f);
        float r = POND_RADIUS - 0.5f; 
        drawReeds(cos(ang*PI/180)*r, sin(ang*PI/180)*r);
    }
    
    drawGazebo(); 
    drawCat();     
    
    //   LOOPING MELINGKAR
    //  menggunakan Loop dan Trigonometri untuk menyusun lampu dan bangku secara otomatis mengelilingi kolam."
    int numItems = 8;
    for(int i=0; i<numItems; i++) {
        float angle = i * (360.0f / numItems) * (PI / 180.0f);
        float rLamp = PATH_OUTER_R + 0.8f; 
        
        // Selang-seling Lampu dan Bangku
        if (i % 2 == 0) {
            drawStreetLamp(cos(angle)*rLamp, sin(angle)*rLamp, -i * (360.0f/numItems) - 90);
        } else {
            drawGardenBench(cos(angle)*rLamp, sin(angle)*rLamp, -i * (360.0f/numItems) + 90);
        }
    }

    for (const auto& t : trees) {
        drawTree(t.x, t.z, t.height, t.trunkWidth);
    }

    glutSwapBuffers();
}

void timer(int v) {
    waterAnim += 0.03f; // Variabel waktu untuk semua animasi
    updateCamera();     
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); 
}

void keyboard(unsigned char key, int x, int y) {
    keys[key] = true;
    if (key == 27) exit(0); 
    
    if (key == 't' || key == 'T') {
        lightMode = (lightMode == 1) ? 2 : 1;
        isSun = (lightMode == 1); 
        std::cout << "Suasana: " << (lightMode == 1 ? "SIANG" : "MALAM") << std::endl;
    }
    
    if (key == 'C' || key == 'c') {
        lampOn = !lampOn; 
    }
}

void keyboardUp(unsigned char key, int x, int y) { keys[key] = false; }

void mouseMotion(int x, int y) {
    if (!mouseActive) return;
    int centerX = windowWidth / 2;
    int centerY = windowHeight / 2;
    float deltaX = (x - centerX) * MOUSE_SENSITIVITY * 40.0f;
    float deltaY = (y - centerY) * MOUSE_SENSITIVITY * 40.0f;
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

void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / h, 0.1, 1500.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Taman umum TB");
    initLighting();
    initTrees(); 
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);
    glutMouseFunc(mouse);
    
    glutTimerFunc(0, timer, 0);

    std::cout << " === KONTROL TAMAN === " << std::endl;
    std::cout << " [ W, A, S, D ] Gerak Kamera" << std::endl;
    std::cout << " [ Klik Kiri ]  Mouse Look" << std::endl;
    std::cout << " [ T ]          Siang/Malam" << std::endl;
    std::cout << " [ C ]          Lampu" << std::endl;

    glutMainLoop();
    return 0;
}