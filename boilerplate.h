bool InitializeShaders(MyShader);
void DestroyShaders(MyShader);

void InitializeGeometry(MyGeometry);
void RenderGeometry(MyGeometry);
void DestroyGeometry(MyGeometry);

void RenderScene(MyGeometry, MyShader);
void addControlPoints(vector<vec2>, vector<vec2>, vector<vec3>, vector<vec3>, int);
void removeControlPoints(vector<vec2>, vector<vec3>);
void createColors(vector<vec3>, vec3, int);

void drawKettle();
void drawFish();
void drawCall();

float setGlyph(char, vec2);
float setText(string);

void ErrorCallback(int, const char*);

void resetUniforms();

void KeyCallback(GLFWwindow*, int, int, int, int);
void ScrollCallback(GLFWwindow*, double, double);

int main(int, char);

void QueryGLVersion();
bool CheckGLErrors();
string LoadSource(const string);
GLuint CompileShader(GLenum, const string);
GLuint LinkProgram(GLuint, GLuint, GLuint, GLuint);
