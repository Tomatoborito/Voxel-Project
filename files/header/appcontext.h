#pragma once

class Render;
class Inputs;

struct AppContext {
    Render* render = nullptr;
    Inputs* inputs = nullptr;
};