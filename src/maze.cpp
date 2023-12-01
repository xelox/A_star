#include "./maze.h"

//Maze::Point methods

Maze::Point::Point(int x, int y) {
    this->x = x;
    this->y = y;
}
Maze::Point Maze::Point::operator + (const Point& rhs) const {
    return Point(this->x + rhs.x, this->y + rhs.y);
}
Maze::Point Maze::Point::operator * (const int &rhs) const {
    return Point(this->x * rhs, this->y * rhs);
}

bool Maze::Point::operator == (const Point &rhs) const {
    return this->x == rhs.x && this->y == rhs.y;
}

float Maze::Point::dist(const Point &rhs) const {
    return std::sqrt(
        (float)(std::pow(this->x - rhs.x, 2) + std::pow(this->y - rhs.y, 2))
    );
}

Vector2 Maze::Point::vec2() {
    return Vector2({(float)this->x, (float)this->y});
}








//Maze::MazeCell methods

bool Maze::MazeCell::northIsWalled() const { return this->walls & 0b00001000; }
bool Maze::MazeCell::eastIsWalled() const { return this->walls & 0b00000100; }
bool Maze::MazeCell::southIsWalled() const { return this->walls & 0b00000010; }
bool Maze::MazeCell::westIsWalled() const { return this->walls & 0b00000001; }

void Maze::MazeCell::breakNorth() { this->walls &= 0b00000111; }
void Maze::MazeCell::breakEast() { this->walls &= 0b00001011; }
void Maze::MazeCell::breakSouth() { this->walls &= 0b00001101; }
void Maze::MazeCell::breakWest() { this->walls &= 0b00001110; }

void Maze::MazeCell::setState(const CellState &newState) {
    this->state = newState;
}

std::vector<std::pair<Maze::Point, Maze::Point>> Maze::MazeCell::wallLines() const {
    std::vector<std::pair<Point, Point>> wallLinesVec;
    if (this->northIsWalled()) {
        wallLinesVec.push_back(std::pair(Point(0, 0), Point(1, 0)));
    }
    if (this->eastIsWalled()) {
        wallLinesVec.push_back(std::pair(Point(1, 0), Point(1, 1)));
    }
    if (this->southIsWalled()) {
        wallLinesVec.push_back(std::pair(Point(1, 1), Point(0, 1)));
    }
    if (this->westIsWalled()) {
        wallLinesVec.push_back(std::pair(Point(0, 1), Point(0, 0)));
    }
    return wallLinesVec;
}

Color Maze::MazeCell::color() const { return Maze::colors[this->state]; }

Maze::CellState Maze::MazeCell::checkState() const { return this->state; }








// Maze::SolutionUnit methods

bool Maze::SolutionUnit::operator<(const SolutionUnit &rhs) const {
    if (this->fCost < rhs.fCost)
        return true;
    if (this->fCost == rhs.fCost) {
        if (this->hCost < rhs.hCost)
            return true;
        if (this->hCost == rhs.fCost)
            if (this->gCost < rhs.gCost)
                return true;
    }
    return false;
}







// Maze methods

Maze::Maze(unsigned int size, unsigned int clientWidth,
           unsigned int clientHeight) {
    this->cellSize = size;
    this->mazeWidth = (clientWidth - this->mazeMargin * 2) / this->cellSize;
    this->mazeHeight = (clientHeight - this->mazeMargin * 2) / this->cellSize;

    this->startPoint = Point(0, 0);
    this->endPoint = Point(this->mazeWidth - 1, this->mazeHeight - 1);

    rng = std::mt19937(rd());

    resetMaze();
}

Maze::~Maze() {
    delete this->mazeGrid;
    this->mazeGrid = nullptr;
    for (auto &unitptr : this->closedSet)
    delete unitptr;
}

void Maze::resetMaze() {
    this->ready = false;
    this->solved = false;
    this->traced = false;

    if (this->mazeGrid != nullptr)
        delete this->mazeGrid;
    this->mazeGrid = new MazeCell[this->mazeWidth * this->mazeHeight];

    this->tracingPtr = nullptr;

    for (unsigned int x = 0; x < this->mazeWidth; x++) {
        for (unsigned int y = 0; y < this->mazeHeight; y++) {
            this->mazeGrid[y * this->mazeWidth + x].pos = Point(x, y);
        }
    }

    this->mazeGenStack = std::stack<MazeCell *>();
    const auto centerCellIdx = (this->mazeHeight * (this->mazeWidth + 1)) / 2;
    auto startCell = &this->mazeGrid[centerCellIdx];
    startCell->setState(CellState::gen_ready);
    this->mazeGenStack.push(startCell);

    for (auto &ptr : this->closedSet)
    delete ptr;
    this->closedSet.clear();
    this->openSet.clear();

    SolutionUnit startingUnit;
    startingUnit.pos = this->startPoint;
    startingUnit.gCost = 0;
    startingUnit.hCost = startingUnit.pos.dist(this->endPoint);
    startingUnit.fCost = startingUnit.gCost + startingUnit.hCost;
    this->openSet.push_back(startingUnit);
}

void Maze::draw() {
    for (unsigned int x = 0; x < this->mazeWidth; x++) {
        for (unsigned int y = 0; y < this->mazeHeight; y++) {
            auto idx = x + y * this->mazeWidth;
            const auto &cell = this->mazeGrid[idx];

            auto drawPos = Point(x * this->cellSize + this->mazeMargin,
                                 y * this->cellSize + this->mazeMargin);

            DrawRectangle(drawPos.x, drawPos.y, this->cellSize, this->cellSize,
                          cell.color());
            for (const auto &wall : cell.wallLines()) {
                auto p1 = (drawPos + wall.first * this->cellSize).vec2();
                auto p2 = (drawPos + wall.second * this->cellSize).vec2();
                DrawLineV(p1, p2, BLACK);
            }
        }
    }

    if (this->mazeGenStack.empty()) return;

    auto headPos = this->mazeGenStack.top()->pos;
    auto drawPos = Point(headPos.x * this->cellSize + this->mazeMargin,
                         headPos.y * this->cellSize + this->mazeMargin);
    DrawRectangle(drawPos.x, drawPos.y, this->cellSize, this->cellSize, RED);
}

void Maze::generationStep() {
    if (this->mazeGenStack.size() == 0) {
        this->ready = true;
        return;
    }

    MazeCell *current = this->mazeGenStack.top();
    this->mazeGenStack.pop();

    std::vector<std::pair<MazeCell *, Direction>> cutCandidates;

    for (unsigned short d = 0; d < this->nesw.size(); d++) {
        const auto &dir = this->nesw[d];
        const auto candidatePos = current->pos + dir;

        if (candidatePos.x < 0 || candidatePos.y < 0 ||
            candidatePos.x >= this->mazeWidth ||
            candidatePos.y >= this->mazeHeight) {
            continue;
        }

        const auto idx = candidatePos.y * this->mazeWidth + candidatePos.x;
        const auto candidate = &this->mazeGrid[idx];
        if (candidate->checkState() != CellState::initial)
            continue;

        cutCandidates.push_back(std::pair(candidate, (Direction)d));
    }

    if (cutCandidates.size()) {
        this->mazeGenStack.push(current);

        std::uniform_int_distribution<unsigned char> dice(
            0, cutCandidates.size() - 1);
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
                throw "switch statment case not covered";
        }

        choice->setState(CellState::gen_ready);
        this->mazeGenStack.push(choice);
    }
}

void Maze::solvingStep() {
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

    unsigned int gridIdx =
        currentNode.pos.y * this->mazeWidth + currentNode.pos.x;
    auto &currentCell = this->mazeGrid[gridIdx];
    currentCell.setState(CellState::solve_close);

    for (unsigned char d = 0; d < this->nesw.size(); d++) {
        auto dir = this->nesw[d];

        bool canGo = true;
        switch ((Direction)d) {
            case Direction::north:
                if (currentCell.northIsWalled())
                    canGo = false;
                break;
            case Direction::east:
                if (currentCell.eastIsWalled())
                    canGo = false;
                break;
            case Direction::south:
                if (currentCell.southIsWalled())
                    canGo = false;
                break;
            case Direction::west:
                if (currentCell.westIsWalled())
                    canGo = false;
                break;
            default:
                throw "switch statment case not covered";
        }
        if (!canGo)
            continue;

        auto nextCellPos = currentNode.pos + dir;
        unsigned int nextCellIdx =
            nextCellPos.y * this->mazeWidth + nextCellPos.x;
        auto &nextCell = this->mazeGrid[nextCellIdx];

        if (nextCell.checkState() == CellState::solve_close) continue;

        SolutionUnit unit;
        unit.pos = nextCellPos;
        unit.hCost = currentNode.pos.dist(this->endPoint);
        unit.gCost = currentNode.gCost + 1;
        unit.fCost = unit.gCost + unit.hCost;
        unit.parent = currentNodePtr;

        auto existingOpenSolUnitPtr = this->findOpenNode(nextCellPos);
        if (existingOpenSolUnitPtr != nullptr) {
            if (unit < *existingOpenSolUnitPtr)
                *existingOpenSolUnitPtr = unit;
        } else {
            nextCell.setState(CellState::solve_open);
            this->openSet.push_back(unit);
        }
    }
}

void Maze::tracingStep() {
    if (this->tracingPtr == nullptr) {
        this->traced = true;
        return;
    }
    unsigned int cellIdx =
        this->tracingPtr->pos.y * this->mazeWidth + this->tracingPtr->pos.x;
    auto &cell = this->mazeGrid[cellIdx];
    cell.setState(CellState::solution);
    this->tracingPtr = this->tracingPtr->parent;
}

bool Maze::isReady() const { return this->ready; }

bool Maze::isSolved() const { return this->solved; }

bool Maze::isTraced() const { return this->traced; }

Maze::SolutionUnit *Maze::popNextBestSolutionStep() {
    auto best = this->openSet[0];
    unsigned int bestIdx = 0;

    for (unsigned int i = 0; i < this->openSet.size(); i++) {
        const auto &candidate = this->openSet[i];
        if (candidate < best) {
            best = candidate;
            bestIdx = i;
        }
    }

    this->openSet.erase(this->openSet.begin() + bestIdx);

    SolutionUnit *closed = new SolutionUnit(best);
    this->closedSet.push_back(closed);
    return closed;
}

Maze::SolutionUnit *Maze::findOpenNode(const Point &pos) {
    for (auto &node : this->openSet) {
        if (node.pos == pos)
            return &node;
    }
    return nullptr;
}

