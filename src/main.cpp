#include <raylib.h>
#include "./maze.h"



int main(int argc, char** argv) {
    const unsigned int clientWidth = 1200;
    const unsigned int clientHeight = 900;

    Maze maze(20, clientWidth, clientHeight);
    while (!maze.isReady()) maze.generationStep();

    InitWindow(clientWidth, clientHeight, "raylibWin");
    SetTargetFPS(165);
    SetTraceLogLevel(TraceLogLevel::LOG_ERROR);
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_UNFOCUSED);
    bool running = false;

    while( !WindowShouldClose() ) {
        BeginDrawing();
        ClearBackground(WHITE);
        DrawFPS(10, 10);
        maze.draw();

        if (IsKeyPressed(KEY_SPACE)) running = true;
        if (IsKeyPressed(KEY_R)) {
            maze.resetMaze();
            running = false;
            while (!maze.isReady()) maze.generationStep();
        }
        if (IsKeyPressed(KEY_T)) {
            maze.resetMaze();
            running = false;
        }

        if (running) {
            if (!maze.isReady()) maze.generationStep();
            else if (!maze.isSolved()) maze.solvingStep();
            else if (!maze.isTraced()) maze.tracingStep();
            else maze.resetMaze();
        }
      
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
