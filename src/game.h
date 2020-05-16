#define MAX_SEGMENT_SIZE 30

enum side_ {
    Side_Left,
    Side_Right
};

struct branch {
    v2 P[5000];
    r32 V[5000];
};

struct segment {
    b32 HasBranches;
    v2 Top[2];

    r32 GrowRate[2];
    r32 Density;
    r32 Elasticity;

    r32 LeftBranchPoint;
    u32 LeftBranchCount;
    segment *LeftBranch;

    r32 RightBranchPoint;
    u32 RightBranchCount;
    segment *RightBranch;

    // attachment
    segment *Parent;
    int Side; // 0-top, 1-left side, 2-right side
};

struct plant {
    b32 Flag;

    segment Segments[1024];
    u32 SegmentCount;

    // r32 Light;
    // r32 Sugar;

    void GrowBranch(r32 dT, segment *Segments, u32 *SegmentCount, r32 MaxRate, b32 BranchesAllowed);
};

struct game {
    u32 HelloWortd;
    b32 Initialized;
    r32 Time;
    // ui_id Hovered;
    // ui_id Clicked;

    rect Container;
    rect Soil;

    v2 CameraP;
    r32 CameraScale = 1.0f;

    plant Plant;
    r32 Vx;

    branch TestPlant;
    u32 BranchCount;

    u32 Steps;
    u32 Timer;

    r32 GrowRate = 1.0f;
};

game Game = {};
