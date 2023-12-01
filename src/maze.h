#include <array>
#include <raylib.h>
#include <stack>
#include <vector>
#include <random>

class Maze {
private:
    // Maze internal data structures/utilities;
    struct Point {
        int x;
        int y;

        Point(int x, int y);
        Point() { }

        Point operator + (const Point& rhs) const;
        Point operator*(const int &rhs) const;
        bool operator==(const Point &rhs) const;
        float dist(const Point &rhs) const;
        Vector2 vec2();
    };

    // [WARNING] make sure that Maze::CellState enum and Maze::colors array match!
    enum CellState {
        initial     = 0,
        gen_ready   = 1,
        solve_open  = 2,
        solve_close = 3,
        solution    = 4,
    };

    static constexpr Color colors[5] {
        BLACK,                  //0 initial 
        WHITE,                  //1 gen_ready
        {252, 248, 171, 255},   //2 solve_open 
        {252, 108, 106, 255},   //3 solve_close
        {30, 30, 30, 255},      //4 solution
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

        bool northIsWalled() const;
        bool eastIsWalled() const;
        bool southIsWalled() const;
        bool westIsWalled() const;

        void breakNorth();
        void breakEast();
        void breakSouth();
        void breakWest();

        void setState(const CellState &newState);

        std::vector<std::pair<Point, Point>> wallLines() const;

        Color color() const;

        CellState checkState() const;
    };

    struct SolutionUnit {
        float gCost;
        float hCost;
        float fCost;
        Point pos;
        SolutionUnit* parent = nullptr;

        bool operator<(const SolutionUnit &rhs) const;
    };

    //core maze fields
    unsigned int mazeWidth;
    unsigned int mazeHeight;
    unsigned short cellSize;
    unsigned short mazeMargin = 40;
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
    Maze(unsigned int size, unsigned int clientWidth, unsigned int clientHeight);

    ~Maze();

    void resetMaze();

    void draw();

    void generationStep();

    void solvingStep();

    void tracingStep();

    bool isReady() const;
    bool isSolved() const;
    bool isTraced() const;

private:
    SolutionUnit *popNextBestSolutionStep();

    SolutionUnit *findOpenNode(const Point &pos);
};
