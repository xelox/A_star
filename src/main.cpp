#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
#include <random>

const unsigned int clientWidth = 1200;
const unsigned int clientHeight = 900;

class Maze {
private:
    // Maze internal data structures/utilities;
    struct Point {
        int x;
        int y;

        Point(int x, int y) {
            this->x = x;
            this->y = y;
        }
        Point() { }

        Point operator + (const Point& rhs) const {
            return Point(this->x + rhs.x, this->y + rhs.y);
        }

        Point operator * (const int& rhs) const {
            return Point(this->x * rhs, this->y * rhs);
        }

        bool operator == (const Point& rhs) const {
            return this->x == rhs.x && this->y == rhs.y;
        }

        float dist (const Point& rhs) const {
            return std::sqrt(
                (float)( std::pow(this->x - rhs.x, 2) +
                std::pow(this->y - rhs.y, 2))
            );
        }

        Vector2 vec2() {
            return Vector2({(float)this->x, (float)this->y});
        }
    };

    enum CellState {
        initial     = 0,
        gen_ready   = 1,
        solve_open  = 2,
        solve_close = 3,
        solution    = 4,
    };

    enum Direction {
        north   = 0,
        east    = 1,
        south   = 2,
        west    = 3,
    };

    class MazeCell {
    private:
        CellState state = CellState::initial;
        unsigned char walls = 0b00001111;

    public:
        Point pos;

        bool northIsWalled() const { return this->walls & 0b00001000; } 
        bool eastIsWalled() const { return this->walls  & 0b00000100; }
        bool southIsWalled() const { return this->walls & 0b00000010; }
        bool westIsWalled() const { return this->walls  & 0b00000001; }

        void breakNorth() { this->walls &=  0b00000111; }
        void breakEast() { this->walls  &=  0b00001011; }
        void breakSouth() { this->walls &=  0b00001101; }
        void breakWest() { this->walls  &=  0b00001110; }

        void setState(const CellState& newState) {
            this->state = newState;
        }

        std::vector<std::pair<Point, Point>> wallLines() const {
            std::vector<std::pair<Point, Point>> wallLinesVec;
            if (this->northIsWalled()) wallLinesVec.push_back(std::pair(Point(0, 0), Point(1, 0)));
            if (this->eastIsWalled())  wallLinesVec.push_back(std::pair(Point(1, 0), Point(1, 1)));
            if (this->southIsWalled()) wallLinesVec.push_back(std::pair(Point(1, 1), Point(0, 1)));
            if (this->westIsWalled())  wallLinesVec.push_back(std::pair(Point(0, 1), Point(0, 0)));
            return wallLinesVec;
        } 
        
        Color color() const {
            switch (this->state) {
                case CellState::initial:
                    return BLACK;
                    break;
                case CellState::gen_ready:
                    return WHITE;
                    break;
                case CellState::solve_open:
                    return Color({252, 248, 171, 255});
                    break;
                case CellState::solve_close:
                    return Color({252, 108, 106, 255});
                    break;
                case CellState::solution:
                    return RED;
                    break;
                default:
                    exit(404);
            }
        }

        CellState checkState() const {
            return this->state;
        }
    };

    struct SolutionUnit {
        float gCost;
        float hCost;
        float fCost;
        Point pos;
        SolutionUnit* parent = nullptr;

        bool operator < (const SolutionUnit& rhs) const {
            if (this->fCost < rhs.fCost) return true;
            if (this->fCost == rhs.fCost) {
                if (this->hCost < rhs.hCost) return true;
                if (this->hCost == rhs.fCost)
                    if (this->gCost < rhs.gCost) return true;
            }
            return false;
        }
    };

    //core maze fields
    unsigned int mazeWidth;
    unsigned int mazeHeight;
    unsigned short cellSize;
    unsigned short mazeMargin;
    bool ready = false;
    bool solved = false;
    bool traced = false;

    //maze cells
    MazeCell* mazeGrid = nullptr;

    //maze generation data
    std::stack<MazeCell*> mazeGenStack;

    // maze solving data
    std::vector<SolutionUnit> openSet;
    // NOTE: storing the closed set as ptrs to heap is neccessary when backtracing the path.
    std::vector<SolutionUnit*> closedSet; 
    Point endPoint;
    Point startPoint;
    SolutionUnit* tracingPtr = nullptr;
    // directions: North, East, South, West
    const std::array<Point, 4> nesw = {
        Point(0, -1),
        Point(1,  0),
        Point(0,  1),
        Point(-1, 0),
    };

    // randomness utilities;
    std::random_device rd;
    std::mt19937 rng;

public:
    Maze(unsigned int width, unsigned short margin) {
        this->mazeMargin = margin;
        this->mazeWidth = width;
        this->cellSize = (clientWidth - this->mazeMargin * 2) / width;
        this->mazeHeight = (clientHeight - this->mazeMargin * 2) / this->cellSize;
        
        this->startPoint = Point(0, 0);
        this->endPoint = Point(this->mazeWidth - 1, this->mazeHeight - 1);

        rng = std::mt19937(rd());

        resetMaze();
    }
    
    ~Maze() {
        delete this->mazeGrid;
        this->mazeGrid = nullptr;
        for (auto& unitptr : this->closedSet) delete unitptr;
    }
    
    void resetMaze() {
        this->ready = false;
        this->solved = false;
        this->traced = false;

        if (this->mazeGrid != nullptr) delete this->mazeGrid;
        this->mazeGrid = new MazeCell[this->mazeWidth * this->mazeHeight];
        
        this->tracingPtr = nullptr;

        for (unsigned int x = 0; x < this->mazeWidth; x++)
            for (unsigned int y = 0; y < this->mazeHeight; y++) {
                this->mazeGrid[y * this->mazeWidth + x].pos = Point(x, y);
            }

        this->mazeGenStack = std::stack<MazeCell*>();
        auto startCell = &this->mazeGrid[this->mazeHeight / 2 * this->mazeWidth + this->mazeWidth / 2];
        startCell->setState(CellState::gen_ready);
        this->mazeGenStack.push(startCell);

        for (auto& ptr : this->closedSet) delete ptr;
        this->closedSet.clear();
        this->openSet.clear();
        

        SolutionUnit startingUnit;
        startingUnit.pos = this->startPoint;
        startingUnit.gCost = 0;
        startingUnit.hCost = startingUnit.pos.dist(this->endPoint);
        startingUnit.fCost = startingUnit.gCost + startingUnit.hCost; 
        this->openSet.push_back(startingUnit);
    }

    void draw() {
        for (unsigned int x = 0; x < this->mazeWidth; x++)
            for (unsigned int y = 0; y < this->mazeHeight; y++) {
                auto idx = x + y * this->mazeWidth;
                const auto& cell = this->mazeGrid[idx];

                auto drawPos = Point(x * this->cellSize + this->mazeMargin , y * this->cellSize + this->mazeMargin);

                DrawRectangle(drawPos.x, drawPos.y, this->cellSize, this->cellSize, cell.color());
                for (const auto& wall : cell.wallLines()) {
                    auto p1 = (drawPos + wall.first * this->cellSize).vec2();
                    auto p2 = (drawPos + wall.second * this->cellSize).vec2();
                    DrawLineV(p1, p2, BLACK);
                }
            }
        
        if (this->mazeGenStack.empty()) return;

        auto headPos = this->mazeGenStack.top()->pos;
        auto drawPos = Point(headPos.x * this->cellSize + this->mazeMargin , headPos.y * this->cellSize + this->mazeMargin);
        DrawRectangle(drawPos.x, drawPos.y, this->cellSize, this->cellSize, RED);
    }

    void generationStep() {
        if (this->mazeGenStack.size() == 0) {
            this->ready = true;
            return;
        }

        MazeCell* current = this->mazeGenStack.top();
        this->mazeGenStack.pop();

        std::vector<std::pair<MazeCell*, Direction>> cutCandidates;

        for (unsigned short d = 0; d < this->nesw.size(); d++) {
            const auto& dir = this->nesw[d];
            const auto candidatePos = current->pos + dir;

            if (
                candidatePos.x < 0 ||
                candidatePos.y < 0 ||
                candidatePos.x >= this->mazeWidth ||
                candidatePos.y >= this->mazeHeight
            ) { continue; }

            const auto idx = candidatePos.y * this->mazeWidth + candidatePos.x;
            const auto candidate = &this->mazeGrid[idx];
            if (candidate->checkState() != CellState::initial) continue; 

            cutCandidates.push_back(std::pair(candidate, (Direction)d));
        }

        if (cutCandidates.size()) {
            this->mazeGenStack.push(current);

            std::uniform_int_distribution<unsigned char> dice(0, cutCandidates.size() - 1);
            const short choiceDice = dice(this->rng);
            auto [choice, dir] = cutCandidates[choiceDice];


            switch (dir) {
                case north:
                    current->breakNorth();
                    choice->breakSouth();
                    break;
                case east:
                    current->breakEast();
                    choice->breakWest();
                    break;
                case south:
                    current->breakSouth();
                    choice->breakNorth();
                    break;
                case west:
                    current->breakWest();
                    choice->breakEast();
                    break;
                default:
                    exit(405);
            }

            choice->setState(CellState::gen_ready);
            this->mazeGenStack.push(choice);
        }
    }

    void solvingStep() {
        if (this->openSet.size() == 0) {
            this->solved = true;
            return;
        }
        auto currentNodePtr = this->popNextBestSolutionStep();
        auto currentNode = *currentNodePtr;

        if (currentNode.pos == this->endPoint) {
            this->solved = true;
            this->tracingPtr = currentNodePtr;
            return;
        }

        unsigned int gridIdx = currentNode.pos.y * this->mazeWidth + currentNode.pos.x;
        auto& currentCell = this->mazeGrid[gridIdx];
        currentCell.setState(CellState::solve_close);

        for (unsigned char d = 0; d < this->nesw.size(); d++) {

            auto dir = this->nesw[d];

            bool canGo = true;
            switch ((Direction)d) {
                case Direction::north: 
                    if (currentCell.northIsWalled()) canGo = false;
                    break;
                case Direction::east: 
                    if (currentCell.eastIsWalled()) canGo = false;
                    break;
                case Direction::south: 
                    if (currentCell.southIsWalled()) canGo = false;
                    break;
                case Direction::west: 
                    if (currentCell.westIsWalled()) canGo = false;
                    break;
                default:
                    exit(500);
            }
            if (!canGo) continue;


            auto nextCellPos = currentNode.pos + dir;
            unsigned int nextCellIdx = nextCellPos.y * this->mazeWidth + nextCellPos.x;
            auto& nextCell = this->mazeGrid[nextCellIdx];

            if (nextCell.checkState() == CellState::solve_close) continue;

            SolutionUnit unit;
            unit.pos = nextCellPos;
            unit.hCost = currentNode.pos.dist(this->endPoint);
            unit.gCost = currentNode.gCost + 1;
            unit.fCost = unit.gCost + unit.hCost;
            unit.parent = currentNodePtr;

            auto existingOpenSolUnitPtr = this->findOpenNode(nextCellPos);
            if (existingOpenSolUnitPtr != nullptr) {
                if (unit < *existingOpenSolUnitPtr) *existingOpenSolUnitPtr = unit;
            }
            else {
                nextCell.setState(CellState::solve_open);
                this->openSet.push_back(unit);
            }
        }
    }

    void tracingStep() {
        if (this->tracingPtr == nullptr) {
            this->traced = true;
            return;
        }
        unsigned int cellIdx = this->tracingPtr->pos.y * this->mazeWidth + this->tracingPtr->pos.x;
        auto& cell = this->mazeGrid[cellIdx];
        cell.setState(CellState::solution);
        this->tracingPtr = this->tracingPtr->parent;
    }
    
    bool isReady() const { return this->ready; }
    bool isSolved() const { return this->solved; }
    bool isTraced() const { return this->traced; }

private:
    SolutionUnit* popNextBestSolutionStep() {
        auto best = this->openSet[0];
        unsigned int bestIdx = 0;

        for (unsigned int i = 0; i < this->openSet.size(); i++) {
            const auto& candidate = this->openSet[i];
            if (candidate < best) {
                best = candidate;
                bestIdx = i;
            }
        }

        this->openSet.erase(this->openSet.begin() + bestIdx);

        SolutionUnit* closed = new SolutionUnit(best);
        this->closedSet.push_back(closed);
        return closed;
    }

    SolutionUnit* findOpenNode(const Point& pos) {
        for (auto& node : this->openSet) {
            if (node.pos == pos) return &node;
        }
        return nullptr;
    }
};

int main(int argc, char** argv) {
    Maze maze(50, 40);
    while (!maze.isReady()) maze.generationStep();

    InitWindow(clientWidth, clientHeight, "raylibWin");
    SetTargetFPS(165);
    SetTraceLogLevel(TraceLogLevel::LOG_ERROR);
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_UNFOCUSED);
    bool running = false;
    std::stringstream elapsed("ready");

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
