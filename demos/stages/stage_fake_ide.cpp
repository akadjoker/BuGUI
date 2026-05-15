#include "WidgetApp.hpp"
#include "BasicWidgets.hpp"
#include "LayoutWidgets.hpp"
#include "DockPanel.hpp"
#include "TreePropertyColorWidgets.hpp"
#include "TextInputWidgets.hpp"
#include "ConsoleWidget.hpp"
#include "DataWidgets.hpp"
#include "MenuWidgets.hpp"
#include "ScrollWidgets.hpp"
#include "InputWidgets.hpp"
#include <cstdio>
using namespace BuGUI;

// ─────────────────────────────────────────────────────────────────────────────
//  Stage — Fake IDE
//
//  Stress tests:
//  - DockPanel with multiple panels
//  - TextEdit with large source code + syntax highlighting markers
//  - TreeView as file explorer (~80+ nodes deep hierarchy)
//  - TabLayout with multiple open "files"
//  - ConsoleWidget (build output)
//  - DataGrid for "Problems" panel (compiler errors/warnings)
//  - PropertyGrid for "Settings"
//  - MenuBar with typical IDE menus
// ─────────────────────────────────────────────────────────────────────────────

static void buildFileTree(TreeView* tree)
{
    auto* root = tree->addRoot("bugui-project");
    root->setExpanded(true);

    // src/
    auto* src = root->addChild("src");
    src->setExpanded(true);
    auto* core = src->addChild("core");
    core->addChild("Application.cpp");
    core->addChild("Application.hpp");
    core->addChild("Logger.cpp");
    core->addChild("Logger.hpp");
    core->addChild("EventSystem.cpp");
    core->addChild("EventSystem.hpp");
    core->addChild("Timer.cpp");
    core->addChild("Timer.hpp");
    core->addChild("Memory.cpp");
    core->addChild("Memory.hpp");
    core->setExpanded(true);

    auto* renderer = src->addChild("renderer");
    renderer->addChild("Renderer.cpp");
    renderer->addChild("Renderer.hpp");
    renderer->addChild("Shader.cpp");
    renderer->addChild("Shader.hpp");
    renderer->addChild("Texture.cpp");
    renderer->addChild("Texture.hpp");
    renderer->addChild("VertexBuffer.cpp");
    renderer->addChild("VertexBuffer.hpp");
    renderer->addChild("FrameBuffer.cpp");
    renderer->addChild("FrameBuffer.hpp");
    renderer->addChild("Camera.cpp");
    renderer->addChild("Camera.hpp");
    renderer->addChild("Material.cpp");
    renderer->addChild("Material.hpp");
    renderer->addChild("RenderPass.cpp");
    renderer->addChild("RenderPass.hpp");
    renderer->setExpanded(true);

    auto* scene = src->addChild("scene");
    scene->addChild("Scene.cpp");
    scene->addChild("Scene.hpp");
    scene->addChild("Entity.cpp");
    scene->addChild("Entity.hpp");
    scene->addChild("Component.hpp");
    scene->addChild("TransformComponent.hpp");
    scene->addChild("MeshComponent.hpp");
    scene->addChild("LightComponent.hpp");
    scene->addChild("ScriptComponent.hpp");
    scene->addChild("PhysicsComponent.hpp");
    scene->setExpanded(true);

    auto* ui = src->addChild("ui");
    ui->addChild("EditorUI.cpp");
    ui->addChild("EditorUI.hpp");
    ui->addChild("InspectorPanel.cpp");
    ui->addChild("InspectorPanel.hpp");
    ui->addChild("SceneHierarchy.cpp");
    ui->addChild("SceneHierarchy.hpp");
    ui->addChild("ContentBrowser.cpp");
    ui->addChild("ContentBrowser.hpp");
    ui->addChild("ConsolePanel.cpp");
    ui->addChild("ConsolePanel.hpp");
    ui->addChild("Toolbar.cpp");
    ui->addChild("Toolbar.hpp");

    auto* physics = src->addChild("physics");
    physics->addChild("PhysicsWorld.cpp");
    physics->addChild("PhysicsWorld.hpp");
    physics->addChild("Collider.cpp");
    physics->addChild("Collider.hpp");
    physics->addChild("RigidBody.cpp");
    physics->addChild("RigidBody.hpp");

    auto* scripting = src->addChild("scripting");
    scripting->addChild("ScriptEngine.cpp");
    scripting->addChild("ScriptEngine.hpp");
    scripting->addChild("LuaBindings.cpp");
    scripting->addChild("LuaBindings.hpp");
    scripting->addChild("ScriptRegistry.cpp");
    scripting->addChild("ScriptRegistry.hpp");

    auto* audio = src->addChild("audio");
    audio->addChild("AudioEngine.cpp");
    audio->addChild("AudioEngine.hpp");
    audio->addChild("AudioSource.cpp");
    audio->addChild("AudioSource.hpp");

    src->addChild("main.cpp");
    src->addChild("pch.hpp");

    // include/
    auto* inc = root->addChild("include");
    inc->addChild("Engine.hpp");
    inc->addChild("Types.hpp");
    inc->addChild("Math.hpp");
    inc->addChild("Platform.hpp");

    // assets/
    auto* assets = root->addChild("assets");
    auto* shaders = assets->addChild("shaders");
    shaders->addChild("pbr.vert");
    shaders->addChild("pbr.frag");
    shaders->addChild("skybox.vert");
    shaders->addChild("skybox.frag");
    shaders->addChild("shadow.vert");
    shaders->addChild("shadow.frag");
    shaders->addChild("post_process.frag");
    shaders->addChild("blur.comp");
    auto* textures = assets->addChild("textures");
    for (int i = 0; i < 12; ++i) {
        char buf[32]; snprintf(buf, sizeof(buf), "texture_%02d.png", i);
        textures->addChild(buf);
    }
    auto* models = assets->addChild("models");
    models->addChild("player.fbx");
    models->addChild("terrain.obj");
    models->addChild("building_01.gltf");
    models->addChild("tree_pine.fbx");
    auto* sounds = assets->addChild("sounds");
    sounds->addChild("bgm.ogg");
    sounds->addChild("footstep.wav");
    sounds->addChild("explosion.wav");

    // scripts/
    auto* scripts = root->addChild("scripts");
    scripts->addChild("player_controller.lua");
    scripts->addChild("enemy_ai.lua");
    scripts->addChild("camera_follow.lua");
    scripts->addChild("inventory.lua");
    scripts->addChild("dialogue_system.lua");
    scripts->addChild("save_load.lua");
    scripts->addChild("particle_effects.lua");

    // tests/
    auto* tests = root->addChild("tests");
    tests->addChild("test_renderer.cpp");
    tests->addChild("test_physics.cpp");
    tests->addChild("test_scene.cpp");
    tests->addChild("test_scripting.cpp");
    tests->addChild("test_audio.cpp");
    tests->addChild("test_math.cpp");

    // Build files
    root->addChild("CMakeLists.txt");
    root->addChild("README.md");
    root->addChild(".gitignore");
    root->addChild("vcpkg.json");
}

// Source code for the "open file" TextEdit
static const char* sampleCode =
    "#include \"Renderer.hpp\"\n"
    "#include \"Shader.hpp\"\n"
    "#include \"Camera.hpp\"\n"
    "#include \"Material.hpp\"\n"
    "#include \"VertexBuffer.hpp\"\n"
    "#include \"FrameBuffer.hpp\"\n"
    "#include <glad/glad.h>\n"
    "#include <glm/glm.hpp>\n"
    "#include <glm/gtc/matrix_transform.hpp>\n"
    "#include <algorithm>\n"
    "#include <cassert>\n"
    "\n"
    "namespace Engine {\n"
    "\n"
    "// ═══════════════════════════════════════════════════════════\n"
    "//  Renderer Implementation\n"
    "// ═══════════════════════════════════════════════════════════\n"
    "\n"
    "Renderer::Renderer()\n"
    "    : clearColor_(0.1f, 0.1f, 0.12f, 1.0f)\n"
    "    , wireframe_(false)\n"
    "    , drawCalls_(0)\n"
    "    , triangles_(0)\n"
    "{\n"
    "}\n"
    "\n"
    "Renderer::~Renderer() {\n"
    "    shutdown();\n"
    "}\n"
    "\n"
    "bool Renderer::init(int width, int height) {\n"
    "    if (!gladLoadGL()) {\n"
    "        LOG_ERROR(\"Failed to initialise OpenGL\");\n"
    "        return false;\n"
    "    }\n"
    "\n"
    "    width_  = width;\n"
    "    height_ = height;\n"
    "\n"
    "    glEnable(GL_DEPTH_TEST);\n"
    "    glEnable(GL_CULL_FACE);\n"
    "    glCullFace(GL_BACK);\n"
    "    glFrontFace(GL_CCW);\n"
    "    glEnable(GL_BLEND);\n"
    "    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);\n"
    "\n"
    "    // Shadow map framebuffer\n"
    "    shadowFBO_ = std::make_unique<FrameBuffer>(2048, 2048,\n"
    "        FrameBuffer::Depth);\n"
    "\n"
    "    // HDR framebuffer\n"
    "    hdrFBO_ = std::make_unique<FrameBuffer>(width, height,\n"
    "        FrameBuffer::Color | FrameBuffer::Depth);\n"
    "\n"
    "    // Post-processing\n"
    "    postShader_ = Shader::load(\"assets/shaders/post_process.frag\");\n"
    "    blurShader_ = Shader::loadCompute(\"assets/shaders/blur.comp\");\n"
    "\n"
    "    // Quad for fullscreen pass\n"
    "    float quadVerts[] = {\n"
    "        -1.f, -1.f,  0.f, 0.f,\n"
    "         1.f, -1.f,  1.f, 0.f,\n"
    "         1.f,  1.f,  1.f, 1.f,\n"
    "        -1.f,  1.f,  0.f, 1.f\n"
    "    };\n"
    "    uint32_t quadIdx[] = { 0, 1, 2,  2, 3, 0 };\n"
    "    quadVAO_ = VertexBuffer::create(quadVerts, sizeof(quadVerts),\n"
    "                                     quadIdx, 6);\n"
    "\n"
    "    LOG_INFO(\"Renderer initialised ({}x{})\", width, height);\n"
    "    return true;\n"
    "}\n"
    "\n"
    "void Renderer::shutdown() {\n"
    "    shadowFBO_.reset();\n"
    "    hdrFBO_.reset();\n"
    "    quadVAO_.reset();\n"
    "    postShader_.reset();\n"
    "    blurShader_.reset();\n"
    "    LOG_INFO(\"Renderer shut down\");\n"
    "}\n"
    "\n"
    "void Renderer::beginFrame() {\n"
    "    drawCalls_ = 0;\n"
    "    triangles_ = 0;\n"
    "    glClearColor(clearColor_.r, clearColor_.g,\n"
    "                 clearColor_.b, clearColor_.a);\n"
    "    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);\n"
    "}\n"
    "\n"
    "void Renderer::endFrame() {\n"
    "    // Bloom pass\n"
    "    if (bloom_) {\n"
    "        blurShader_->bind();\n"
    "        for (int i = 0; i < bloomPasses_; ++i) {\n"
    "            blurShader_->setInt(\"horizontal\", 1);\n"
    "            blurShader_->dispatch(width_ / 16, height_ / 16, 1);\n"
    "            blurShader_->setInt(\"horizontal\", 0);\n"
    "            blurShader_->dispatch(width_ / 16, height_ / 16, 1);\n"
    "        }\n"
    "    }\n"
    "\n"
    "    // Tone mapping + gamma\n"
    "    postShader_->bind();\n"
    "    postShader_->setFloat(\"exposure\", exposure_);\n"
    "    postShader_->setFloat(\"gamma\", gamma_);\n"
    "    hdrFBO_->bindColorTexture(0);\n"
    "    quadVAO_->draw();\n"
    "}\n"
    "\n"
    "void Renderer::submit(const RenderCommand& cmd) {\n"
    "    assert(cmd.mesh && \"RenderCommand must have a mesh\");\n"
    "    assert(cmd.material && \"RenderCommand must have a material\");\n"
    "\n"
    "    auto& mat = *cmd.material;\n"
    "    auto& shader = *mat.shader();\n"
    "    shader.bind();\n"
    "    shader.setMat4(\"u_model\", cmd.transform);\n"
    "    shader.setMat4(\"u_view\", camera_->viewMatrix());\n"
    "    shader.setMat4(\"u_proj\", camera_->projMatrix());\n"
    "    shader.setVec3(\"u_camPos\", camera_->position());\n"
    "\n"
    "    // Bind textures\n"
    "    int slot = 0;\n"
    "    if (mat.albedoTex())    mat.albedoTex()->bind(slot++);\n"
    "    if (mat.normalTex())    mat.normalTex()->bind(slot++);\n"
    "    if (mat.roughnessTex()) mat.roughnessTex()->bind(slot++);\n"
    "    if (mat.metallicTex())  mat.metallicTex()->bind(slot++);\n"
    "    if (mat.aoTex())        mat.aoTex()->bind(slot++);\n"
    "\n"
    "    // Lights\n"
    "    for (int i = 0; i < std::min((int)lights_.size(), 4); ++i) {\n"
    "        char buf[64];\n"
    "        snprintf(buf, sizeof(buf), \"u_lights[%d].position\", i);\n"
    "        shader.setVec3(buf, lights_[i].position);\n"
    "        snprintf(buf, sizeof(buf), \"u_lights[%d].color\", i);\n"
    "        shader.setVec3(buf, lights_[i].color * lights_[i].intensity);\n"
    "    }\n"
    "\n"
    "    cmd.mesh->draw();\n"
    "    drawCalls_++;\n"
    "    triangles_ += cmd.mesh->triangleCount();\n"
    "}\n"
    "\n"
    "void Renderer::resize(int w, int h) {\n"
    "    if (w == width_ && h == height_) return;\n"
    "    width_  = w;\n"
    "    height_ = h;\n"
    "    glViewport(0, 0, w, h);\n"
    "    hdrFBO_->resize(w, h);\n"
    "    LOG_INFO(\"Renderer resized to {}x{}\", w, h);\n"
    "}\n"
    "\n"
    "} // namespace Engine\n";

// Build output for the console
static void fillBuildOutput(ConsoleWidget* con)
{
    con->log(LogLevel::Info,  "[1/24] Building CXX core/Application.cpp");
    con->log(LogLevel::Info,  "[2/24] Building CXX core/Logger.cpp");
    con->log(LogLevel::Info,  "[3/24] Building CXX core/EventSystem.cpp");
    con->log(LogLevel::Info,  "[4/24] Building CXX core/Timer.cpp");
    con->log(LogLevel::Info,  "[5/24] Building CXX core/Memory.cpp");
    con->log(LogLevel::Info,  "[6/24] Building CXX renderer/Renderer.cpp");
    con->log(LogLevel::Warn,  "renderer/Renderer.cpp:87: warning: unused variable 'slot' [-Wunused-variable]");
    con->log(LogLevel::Info,  "[7/24] Building CXX renderer/Shader.cpp");
    con->log(LogLevel::Info,  "[8/24] Building CXX renderer/Texture.cpp");
    con->log(LogLevel::Info,  "[9/24] Building CXX renderer/VertexBuffer.cpp");
    con->log(LogLevel::Info,  "[10/24] Building CXX renderer/FrameBuffer.cpp");
    con->log(LogLevel::Info,  "[11/24] Building CXX renderer/Camera.cpp");
    con->log(LogLevel::Info,  "[12/24] Building CXX renderer/Material.cpp");
    con->log(LogLevel::Info,  "[13/24] Building CXX renderer/RenderPass.cpp");
    con->log(LogLevel::Info,  "[14/24] Building CXX scene/Scene.cpp");
    con->log(LogLevel::Info,  "[15/24] Building CXX scene/Entity.cpp");
    con->log(LogLevel::Warn,  "scene/Entity.cpp:42: warning: comparison of integers of different signs [-Wsign-compare]");
    con->log(LogLevel::Info,  "[16/24] Building CXX physics/PhysicsWorld.cpp");
    con->log(LogLevel::Info,  "[17/24] Building CXX physics/Collider.cpp");
    con->log(LogLevel::Info,  "[18/24] Building CXX physics/RigidBody.cpp");
    con->log(LogLevel::Info,  "[19/24] Building CXX scripting/ScriptEngine.cpp");
    con->log(LogLevel::Error, "scripting/ScriptEngine.cpp:156: error: 'lua_State' was not declared in this scope");
    con->log(LogLevel::Error, "scripting/ScriptEngine.cpp:157: error: 'luaL_newstate' was not declared in this scope");
    con->log(LogLevel::Error, "scripting/ScriptEngine.cpp:158: error: 'luaL_openlibs' was not declared in this scope");
    con->log(LogLevel::Info,  "[20/24] Building CXX scripting/LuaBindings.cpp");
    con->log(LogLevel::Warn,  "scripting/LuaBindings.cpp:23: warning: implicit conversion loses precision [-Wconversion]");
    con->log(LogLevel::Info,  "[21/24] Building CXX audio/AudioEngine.cpp");
    con->log(LogLevel::Info,  "[22/24] Building CXX audio/AudioSource.cpp");
    con->log(LogLevel::Info,  "[23/24] Building CXX ui/EditorUI.cpp");
    con->log(LogLevel::Info,  "[24/24] Building CXX main.cpp");
    con->log(LogLevel::Error, "FAILED: 3 errors, 3 warnings");
    con->log(LogLevel::Info,  "Build time: 12.4s");
}

void registerFakeIdeStage(WidgetApp& app)
{
    auto* root = app.addStage("fakeide");

    auto* outer = root->createChild<BoxLayout>(LayoutDir::Vertical);
    outer->setStretch(1);
    outer->setSpacing(0);
    outer->setPadding(0);

    // ── Menu Bar ─────────────────────────────────────────────────────────
    auto* menuBar = outer->createChild<MenuBar>();
    {
        auto* fileMenu = menuBar->addMenu("File");
        fileMenu->addAction("New File")->setShortcut("Ctrl+N");
        fileMenu->addAction("Open File...")->setShortcut("Ctrl+O");
        fileMenu->addAction("Open Folder...")->setShortcut("Ctrl+K Ctrl+O");
        static Menu* ideRecentMenu = nullptr;
        if (!ideRecentMenu) {
            ideRecentMenu = new Menu();
            ideRecentMenu->addAction("~/projects/engine");
            ideRecentMenu->addAction("~/projects/bugui");
            ideRecentMenu->addAction("~/projects/game");
            ideRecentMenu->addSeparator();
            ideRecentMenu->addAction("Clear Recently Opened");
        }
        fileMenu->addAction("Open Recent")->setSubmenu(ideRecentMenu);
        fileMenu->addSeparator();
        fileMenu->addAction("Save")->setShortcut("Ctrl+S");
        fileMenu->addAction("Save All")->setShortcut("Ctrl+K S");
        fileMenu->addSeparator();
        fileMenu->addAction("Close Editor")->setShortcut("Ctrl+W");
        fileMenu->addAction("Close Folder");
        fileMenu->addSeparator();
        fileMenu->addAction("Exit");

        auto* editMenu = menuBar->addMenu("Edit");
        editMenu->addAction("Undo")->setShortcut("Ctrl+Z");
        editMenu->addAction("Redo")->setShortcut("Ctrl+Y");
        editMenu->addSeparator();
        editMenu->addAction("Cut")->setShortcut("Ctrl+X");
        editMenu->addAction("Copy")->setShortcut("Ctrl+C");
        editMenu->addAction("Paste")->setShortcut("Ctrl+V");
        editMenu->addSeparator();
        editMenu->addAction("Find")->setShortcut("Ctrl+F");
        editMenu->addAction("Replace")->setShortcut("Ctrl+H");
        editMenu->addAction("Find in Files")->setShortcut("Ctrl+Shift+F");
        editMenu->addSeparator();
        editMenu->addAction("Toggle Comment")->setShortcut("Ctrl+/");
        editMenu->addAction("Format Document")->setShortcut("Shift+Alt+F");

        auto* viewMenu = menuBar->addMenu("View");
        viewMenu->addAction("Explorer")->setShortcut("Ctrl+Shift+E");
        viewMenu->addAction("Search")->setShortcut("Ctrl+Shift+F");
        viewMenu->addAction("Problems")->setShortcut("Ctrl+Shift+M");
        viewMenu->addAction("Output")->setShortcut("Ctrl+Shift+U");
        viewMenu->addAction("Terminal")->setShortcut("Ctrl+`");
        viewMenu->addSeparator();
        viewMenu->addAction("Zoom In")->setShortcut("Ctrl+=");
        viewMenu->addAction("Zoom Out")->setShortcut("Ctrl+-");
        viewMenu->addCheckable("Word Wrap", true)->setShortcut("Alt+Z");
        viewMenu->addCheckable("Minimap", true);

        auto* goMenu = menuBar->addMenu("Go");
        goMenu->addAction("Go to File...")->setShortcut("Ctrl+P");
        goMenu->addAction("Go to Symbol...")->setShortcut("Ctrl+Shift+O");
        goMenu->addAction("Go to Line...")->setShortcut("Ctrl+G");
        goMenu->addAction("Go to Definition")->setShortcut("F12");
        goMenu->addAction("Go to References")->setShortcut("Shift+F12");
        goMenu->addSeparator();
        goMenu->addAction("Back")->setShortcut("Alt+Left");
        goMenu->addAction("Forward")->setShortcut("Alt+Right");

        auto* runMenu = menuBar->addMenu("Run");
        runMenu->addAction("Start Debugging")->setShortcut("F5");
        runMenu->addAction("Run Without Debug")->setShortcut("Ctrl+F5");
        runMenu->addAction("Stop")->setShortcut("Shift+F5");
        runMenu->addSeparator();
        runMenu->addAction("Toggle Breakpoint")->setShortcut("F9");
        runMenu->addAction("Step Over")->setShortcut("F10");
        runMenu->addAction("Step Into")->setShortcut("F11");
        runMenu->addAction("Step Out")->setShortcut("Shift+F11");

        auto* termMenu = menuBar->addMenu("Terminal");
        termMenu->addAction("New Terminal")->setShortcut("Ctrl+Shift+`");
        termMenu->addAction("Split Terminal");
        termMenu->addAction("Run Build Task")->setShortcut("Ctrl+Shift+B");
        termMenu->addAction("Run Task...");

        auto* helpMenu = menuBar->addMenu("Help");
        helpMenu->addAction("Welcome");
        helpMenu->addAction("Keyboard Shortcuts")->setShortcut("Ctrl+K Ctrl+S");
        helpMenu->addAction("About");
    }

    // ── Toolbar ──────────────────────────────────────────────────────────
    auto* toolRow = outer->createChild<BoxLayout>(LayoutDir::Horizontal);
    toolRow->setSize(0, 30);
    toolRow->setPadding(Edges(4, 4, 4, 4));
    toolRow->setSpacing(4);
    {
        auto* back = toolRow->createChild<Button>("\u2190 Menu");
        back->setSize(80, 22);
        back->clicked.connect([&app]() {
            app.setStage("menu", TransitionType::CoverRight);
        });
        toolRow->createChild<Line>();
        toolRow->createChild<Label>("Fake IDE — BuGUI Stress Test");
        toolRow->createChild<Spacer>(0.f)->setStretch(1);

        auto* searchInput = toolRow->createChild<TextInput>();
        searchInput->setPlaceholder("Search files (Ctrl+P)");
        searchInput->setSize(250, 22);

        auto* runBtn = toolRow->createChild<Button>("Run");
        runBtn->setIconId(IconId::Play);
        runBtn->setSize(70, 22);
        auto* stopBtn = toolRow->createChild<Button>("Stop");
        stopBtn->setIconId(IconId::Stop);
        stopBtn->setSize(70, 22);
    }

    // ── DockPanel ────────────────────────────────────────────────────────
    auto* dock = outer->createChild<DockPanel>();
    dock->setStretch(1);

    // File Explorer (TreeView)
    {
        auto* tree = dock->addPanel<TreeView>("Explorer");
        buildFileTree(tree);
    }

    // Editor area: TabLayout with multiple "open files"
    {
        auto* editorTabs = dock->addPanel<TabLayout>("Editor");
        editorTabs->setTabsClosable(true);

        // Tab 1: Renderer.cpp
        {
            auto* tab = editorTabs->addTab<BoxLayout>("Renderer.cpp", LayoutDir::Vertical);
            auto* ed = tab->createChild<TextEdit>();
            ed->setStretch(1);
            ed->setText(sampleCode);
            ed->setMarker(33, TextEdit::MarkerType::Breakpoint);
            ed->setMarker(87, TextEdit::MarkerType::Warning);
            ed->setMarker(116, TextEdit::MarkerType::Error);
        }

        // Tab 2: Entity.cpp
        {
            auto* tab = editorTabs->addTab<BoxLayout>("Entity.cpp", LayoutDir::Vertical);
            auto* ed = tab->createChild<TextEdit>();
            ed->setStretch(1);
            ed->setText(
                "#include \"Entity.hpp\"\n"
                "#include \"Component.hpp\"\n"
                "#include <algorithm>\n"
                "#include <cassert>\n"
                "\n"
                "namespace Engine {\n"
                "\n"
                "Entity::Entity(EntityId id, Scene* scene)\n"
                "    : id_(id), scene_(scene), active_(true)\n"
                "{\n"
                "}\n"
                "\n"
                "Entity::~Entity() {\n"
                "    for (auto& [type, comp] : components_)\n"
                "        delete comp;\n"
                "    components_.clear();\n"
                "}\n"
                "\n"
                "void Entity::addComponent(ComponentType type, Component* comp) {\n"
                "    assert(comp);\n"
                "    assert(components_.find(type) == components_.end());\n"
                "    comp->setOwner(this);\n"
                "    components_[type] = comp;\n"
                "    comp->onAttach();\n"
                "}\n"
                "\n"
                "void Entity::removeComponent(ComponentType type) {\n"
                "    auto it = components_.find(type);\n"
                "    if (it == components_.end()) return;\n"
                "    it->second->onDetach();\n"
                "    delete it->second;\n"
                "    components_.erase(it);\n"
                "}\n"
                "\n"
                "Component* Entity::getComponent(ComponentType type) const {\n"
                "    auto it = components_.find(type);\n"
                "    return (it != components_.end()) ? it->second : nullptr;\n"
                "}\n"
                "\n"
                "void Entity::update(float dt) {\n"
                "    if (!active_) return;\n"
                "    for (auto& [type, comp] : components_) {\n"
                "        if (comp->enabled())\n"
                "            comp->update(dt);\n"
                "    }\n"
                "}\n"
                "\n"
                "bool Entity::hasTag(const std::string& tag) const {\n"
                "    return std::find(tags_.begin(), tags_.end(), tag) != tags_.end();\n"
                "}\n"
                "\n"
                "void Entity::addTag(const std::string& tag) {\n"
                "    if (!hasTag(tag))\n"
                "        tags_.push_back(tag);\n"
                "}\n"
                "\n"
                "} // namespace Engine\n"
            );
            ed->setMarker(42, TextEdit::MarkerType::Warning);
        }

        // Tab 3: CMakeLists.txt
        {
            auto* tab = editorTabs->addTab<BoxLayout>("CMakeLists.txt", LayoutDir::Vertical);
            auto* ed = tab->createChild<TextEdit>();
            ed->setStretch(1);
            ed->setText(
                "cmake_minimum_required(VERSION 3.20)\n"
                "project(GameEngine VERSION 1.0.0 LANGUAGES CXX)\n"
                "\n"
                "set(CMAKE_CXX_STANDARD 17)\n"
                "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
                "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n"
                "\n"
                "# ── Dependencies ────────────────────────────\n"
                "find_package(glfw3 3.3 REQUIRED)\n"
                "find_package(glm REQUIRED)\n"
                "find_package(Lua 5.4 REQUIRED)\n"
                "find_package(OpenAL REQUIRED)\n"
                "\n"
                "# ── Core Library ────────────────────────────\n"
                "file(GLOB_RECURSE ENGINE_SOURCES src/*.cpp)\n"
                "add_library(engine_core STATIC ${ENGINE_SOURCES})\n"
                "target_include_directories(engine_core PUBLIC\n"
                "    ${CMAKE_CURRENT_SOURCE_DIR}/include\n"
                "    ${CMAKE_CURRENT_SOURCE_DIR}/src\n"
                ")\n"
                "target_link_libraries(engine_core PUBLIC\n"
                "    glfw\n"
                "    glm::glm\n"
                "    ${LUA_LIBRARIES}\n"
                "    OpenAL::OpenAL\n"
                ")\n"
                "target_precompile_headers(engine_core PRIVATE src/pch.hpp)\n"
                "\n"
                "# ── Editor Executable ───────────────────────\n"
                "add_executable(editor src/main.cpp)\n"
                "target_link_libraries(editor PRIVATE engine_core)\n"
                "\n"
                "# ── Tests ───────────────────────────────────\n"
                "enable_testing()\n"
                "file(GLOB TEST_SOURCES tests/*.cpp)\n"
                "foreach(test_src ${TEST_SOURCES})\n"
                "    get_filename_component(test_name ${test_src} NAME_WE)\n"
                "    add_executable(${test_name} ${test_src})\n"
                "    target_link_libraries(${test_name} PRIVATE engine_core)\n"
                "    add_test(NAME ${test_name} COMMAND ${test_name})\n"
                "endforeach()\n"
            );
        }

        // Tab 4: player_controller.lua
        {
            auto* tab = editorTabs->addTab<BoxLayout>("player_controller.lua", LayoutDir::Vertical);
            auto* ed = tab->createChild<TextEdit>();
            ed->setStretch(1);
            ed->setText(
                "-- Player Controller Script\n"
                "-- Handles movement, jump, dash, and camera follow\n"
                "\n"
                "local speed = 5.0\n"
                "local jumpForce = 8.0\n"
                "local dashSpeed = 12.0\n"
                "local dashDuration = 0.2\n"
                "local dashCooldown = 1.0\n"
                "\n"
                "local isGrounded = false\n"
                "local isDashing = false\n"
                "local dashTimer = 0\n"
                "local dashCooldownTimer = 0\n"
                "local moveDir = vec3(0, 0, 0)\n"
                "\n"
                "function onStart()\n"
                "    local rb = entity:getComponent(\"RigidBody\")\n"
                "    rb:setMass(80)\n"
                "    rb:setFriction(0.5)\n"
                "    rb:lockRotation(true, false, true)\n"
                "    print(\"Player controller initialised\")\n"
                "end\n"
                "\n"
                "function onUpdate(dt)\n"
                "    local rb = entity:getComponent(\"RigidBody\")\n"
                "    local transform = entity:getComponent(\"Transform\")\n"
                "\n"
                "    -- Input\n"
                "    local h = Input.getAxis(\"Horizontal\")\n"
                "    local v = Input.getAxis(\"Vertical\")\n"
                "    moveDir = vec3(h, 0, v):normalized()\n"
                "\n"
                "    -- Camera-relative movement\n"
                "    local camForward = Camera.main:forward()\n"
                "    camForward.y = 0\n"
                "    camForward = camForward:normalized()\n"
                "    local camRight = Camera.main:right()\n"
                "    camRight.y = 0\n"
                "    camRight = camRight:normalized()\n"
                "\n"
                "    local worldDir = camForward * moveDir.z + camRight * moveDir.x\n"
                "\n"
                "    -- Movement\n"
                "    if not isDashing then\n"
                "        rb:setVelocityXZ(worldDir.x * speed, worldDir.z * speed)\n"
                "    end\n"
                "\n"
                "    -- Rotation (face movement direction)\n"
                "    if worldDir:length() > 0.1 then\n"
                "        local angle = math.atan2(worldDir.x, worldDir.z)\n"
                "        transform:setRotationY(math.deg(angle))\n"
                "    end\n"
                "\n"
                "    -- Ground check\n"
                "    isGrounded = Physics.raycast(\n"
                "        transform:position(),\n"
                "        vec3(0, -1, 0),\n"
                "        1.1\n"
                "    )\n"
                "\n"
                "    -- Jump\n"
                "    if Input.isPressed(\"Jump\") and isGrounded then\n"
                "        rb:addImpulse(vec3(0, jumpForce, 0))\n"
                "    end\n"
                "\n"
                "    -- Dash\n"
                "    dashCooldownTimer = dashCooldownTimer - dt\n"
                "    if Input.isPressed(\"Dash\") and dashCooldownTimer <= 0 then\n"
                "        isDashing = true\n"
                "        dashTimer = dashDuration\n"
                "        dashCooldownTimer = dashCooldown\n"
                "        rb:setVelocity(worldDir * dashSpeed)\n"
                "        Effects.spawn(\"dash_trail\", transform:position())\n"
                "    end\n"
                "\n"
                "    if isDashing then\n"
                "        dashTimer = dashTimer - dt\n"
                "        if dashTimer <= 0 then\n"
                "            isDashing = false\n"
                "        end\n"
                "    end\n"
                "end\n"
                "\n"
                "function onCollision(other)\n"
                "    if other:hasTag(\"enemy\") then\n"
                "        local health = entity:getComponent(\"Health\")\n"
                "        health:damage(10)\n"
                "        Effects.spawn(\"hit_flash\", entity:position())\n"
                "    elseif other:hasTag(\"pickup\") then\n"
                "        local inv = entity:getComponent(\"Inventory\")\n"
                "        inv:addItem(other:getData(\"item_id\"))\n"
                "        other:destroy()\n"
                "    end\n"
                "end\n"
            );
        }

    }

    // Problems panel (DataGrid)
    {
        auto* problems = dock->addPanel<DataGrid>("Problems (9)");
        problems->addColumn("Severity",  70.f, true);
        problems->addColumn("Message",  350.f, false);
        problems->addColumn("File",     180.f, true);
        problems->addColumn("Line",      50.f, true);

        problems->addRow({"Error",   "'lua_State' was not declared in this scope",       "scripting/ScriptEngine.cpp",  "156"});
        problems->addRow({"Error",   "'luaL_newstate' was not declared in this scope",   "scripting/ScriptEngine.cpp",  "157"});
        problems->addRow({"Error",   "'luaL_openlibs' was not declared in this scope",   "scripting/ScriptEngine.cpp",  "158"});
        problems->addRow({"Warning", "unused variable 'slot' [-Wunused-variable]",       "renderer/Renderer.cpp",       "87"});
        problems->addRow({"Warning", "comparison of integers of different signs",        "scene/Entity.cpp",            "42"});
        problems->addRow({"Warning", "implicit conversion loses precision",              "scripting/LuaBindings.cpp",   "23"});
        problems->addRow({"Info",    "Consider using std::string_view for parameter",    "core/Logger.cpp",             "15"});
        problems->addRow({"Info",    "Unused include 'cassert' can be removed",          "renderer/Camera.cpp",         "3"});
        problems->addRow({"Info",    "Function 'resize' can be marked noexcept",         "renderer/FrameBuffer.cpp",    "45"});
        problems->setZebraStripes(true);
    }

    // Build Output (Console)
    {
        auto* con = dock->addPanel<ConsoleWidget>("Output");
        fillBuildOutput(con);
    }

    // Terminal (Console styled as terminal)
    {
        auto* term = dock->addPanel<ConsoleWidget>("Terminal");
        term->log(LogLevel::Info, "$ cd ~/projects/engine");
        term->log(LogLevel::Info, "$ mkdir -p build && cd build");
        term->log(LogLevel::Info, "$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..");
        term->log(LogLevel::Info, "-- The CXX compiler identification is Clang 17.0.6");
        term->log(LogLevel::Info, "-- Found GLFW3: /usr/lib/libglfw.so");
        term->log(LogLevel::Info, "-- Found GLM: /usr/include/glm");
        term->log(LogLevel::Info, "-- Found Lua: /usr/lib/liblua.so (ver 5.4.6)");
        term->log(LogLevel::Info, "-- Configuring done (0.8s)");
        term->log(LogLevel::Info, "-- Generating done (0.1s)");
        term->log(LogLevel::Info, "$ ninja 2>&1");
        term->log(LogLevel::Error, "FAILED: scripting/ScriptEngine.cpp.o");
        term->log(LogLevel::Error, "ninja: build stopped: 3 errors.");
        term->log(LogLevel::Info, "$ _");
    }

    // Settings (PropertyGrid)
    {
        auto* pg = dock->addPanel<PropertyGrid>("Settings");
        pg->addSection("Editor");
        pg->addInt("Font Size", 14, 8, 72);
        pg->addCombo("Font Family", {"JetBrains Mono", "Fira Code", "Consolas", "Cascadia Code", "Source Code Pro"}, 0);
        pg->addInt("Tab Size", 4, 1, 8);
        pg->addBool("Insert Spaces", true);
        pg->addBool("Word Wrap", true);
        pg->addBool("Minimap", true);
        pg->addBool("Line Numbers", true);
        pg->addBool("Bracket Pairs", true);

        pg->addSection("Theme");
        pg->addCombo("Color Theme", {"Dark+", "Monokai", "Dracula", "One Dark Pro", "Solarized Dark", "Nord"}, 0);
        pg->addColor("Editor Background", Color(30, 30, 30, 255));
        pg->addColor("Line Highlight", Color(40, 44, 52, 255));
        pg->addColor("Selection", Color(38, 79, 120, 255));
        pg->addColor("Cursor", Color(220, 220, 220, 255));

        pg->addSection("Files");
        pg->addBool("Auto Save", true);
        pg->addCombo("Auto Save Delay", {"100ms", "500ms", "1s", "5s"}, 2);
        pg->addBool("Format on Save", true);
        pg->addBool("Trim Trailing Whitespace", true);
        pg->addBool("Insert Final Newline", true);
        pg->addString("Exclude Patterns", "*.o,*.d,build/,.git/");

        pg->addSection("Build");
        pg->addString("Build Command", "ninja -C build");
        pg->addString("Run Command", "./build/editor");
        pg->addCombo("Build Type", {"Debug", "Release", "RelWithDebInfo", "MinSizeRel"}, 0);
        pg->addBool("Parallel Build", true);
        pg->addInt("Jobs", 8, 1, 32);

        pg->addSection("Debug");
        pg->addString("Debugger", "lldb");
        pg->addBool("Stop at Entry", false);
        pg->addBool("External Console", false);
        pg->addString("Arguments", "--scene forest.json");
        pg->addString("Working Dir", "${workspaceFolder}");
    }

    // ── Initial layout ───────────────────────────────────────────────────
    dock->splitOff("Explorer",      DockSide::Left,   0.18f);
    dock->splitOff("Settings",      DockSide::Right,  0.22f);
    dock->splitOff("Output",        DockSide::Bottom, 0.25f);
    dock->splitOff("Problems (9)",  DockSide::Bottom, 0.40f);

    // ── Status Bar ───────────────────────────────────────────────────────
    auto* status = outer->createChild<StatusBar>();
    status->createChild<Label>("Ln 87, Col 24");
    status->createChild<Line>();
    status->createChild<Label>("UTF-8");
    status->createChild<Line>();
    status->createChild<Label>("C++17");
    status->createChild<Spacer>(0.f)->setStretch(1);
    status->createChild<Label>("3 Errors, 3 Warnings");
    status->createChild<Line>();
    status->createChild<Label>("Clang 17.0.6");
}
