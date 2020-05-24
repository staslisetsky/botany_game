#include "game.h"
#include "layout.cpp"

void plant::
GrowBranch(r32 dT, segment *Segments, u32 *SegmentCount, r32 MaxRate, b32 BranchesAllowed = true)
{
    r32 A[2] = {};

    for (u32 i=1; i < *SegmentCount; ++i) {
        segment *S = Segments + i;
        segment *P = Segments + i - 1;

        r32 LeftGrowth = S->GrowRate[Side_Left] * dT * 10.0f;
        r32 RightGrowth = S->GrowRate[Side_Right] * dT * 10.0f;

        v2 LeftDirection = Normalize(S->Top[Side_Left] - P->Top[Side_Left]);
        v2 RightDirection = Normalize(S->Top[Side_Right] - P->Top[Side_Right]);

        r32 LeftSize = Length(S->Top[Side_Left] - P->Top[Side_Left]);
        r32 RightSize = Length(S->Top[Side_Right] - P->Top[Side_Right]);

        if ((LeftSize + LeftGrowth) > MAX_SEGMENT_SIZE || (RightSize + RightGrowth) > MAX_SEGMENT_SIZE) {
            Assert(*SegmentCount < 1024);

            segment *New = S + 1;

            u32 TailSize = *SegmentCount - (i + 1);
            if (TailSize) {
                memmove(S + 2, S + 1, TailSize * sizeof(segment));
                segment *Next = S + 2;
            }

            *New = {};

            New->Top[Side_Left] = S->Top[Side_Left];
            New->Top[Side_Right] = S->Top[Side_Right];

            if (i == *SegmentCount - 1) {
                New->GrowRate[Side_Left] = MaxRate;
                New->GrowRate[Side_Right] = MaxRate;
            } else {
                New->GrowRate[Side_Left] = S->GrowRate[Side_Left];
                New->GrowRate[Side_Right] = S->GrowRate[Side_Right];
            }

            S->GrowRate[Side_Left] = MaxRate / 10.0f;
            S->GrowRate[Side_Right] = MaxRate / 10.0f;

            S->Top[Side_Left] += LeftDirection * -1.0f * (LeftSize / 2.0f);
            S->Top[Side_Right] += RightDirection * -1.0f * (RightSize / 2.0f);

            ++*SegmentCount;
            ++i;

            continue;

            S = New;
        }

        if (BranchesAllowed && (i == *SegmentCount - 1) && !S->HasBranches) {
            r32 A = (r32)(rand() % 100) / 100.0f;
            r32 B = (r32)(rand() % 100) / 100.0f;

            this->Plorx.Kront += MaxRate + (A * 10.0f);

            if (this->Plorx.Kront > 20 && !S->LeafCount) {
                this->Plorx.Kront = 0.0f;

                v2 Front = Perp(LeftDirection);
                v2 Side = LeftDirection;
                v2 P = S->Top[Side_Left];

                if (this->Flag) {
                    Front = Perp(RightDirection) * -1.0f;
                    Side = RightDirection;
                    P = S->Top[Side_Right];
                }

                S->Leaves[0].P = P;
                S->Leaves[0].Direction = Front;
                ++S->LeafCount;

                this->Flag = !this->Flag;

                // S->HasBranches = true;
                // segment *Branch = 0;

                // S->LeftBranchPoint = LeftSize / MAX_SEGMENT_SIZE;
                // S->LeftBranch = (segment *)malloc(sizeof(segment) * 1024);

                // S->LeftBranch[0] = {};
                // S->LeftBranch[1] = {};

                // S->LeftBranch[0].Top[Side_Left] =
                // S->LeftBranch[0].Top[Side_Right] = Side * -5.0f + S->Top[Side_Left];

                // S->LeftBranch[1].Top[Side_Left] = Side * 5.0f + Front * 10.0f + S->Top[Side_Left];
                // S->LeftBranch[1].Top[Side_Right] = Side * -5.0f + Front * 10.0f + S->Top[Side_Left];
                // S->LeftBranch[1].GrowRate[Side_Left] = 0.3f;
                // S->LeftBranch[1].GrowRate[Side_Right] = 0.3f;

                // S->LeftBranchCount = 2;
                // S->RightBranchPoint = RightSize / MAX_SEGMENT_SIZE;
                // S->RightBranch = (segment *)malloc(sizeof(segment) * 1024);

                // S->RightBranch[0] = {};
                // S->RightBranch[1] = {};

                // v2 Front = -1.0f * Perp(LeftDirection);
                // v2 Side = RightDirection;

                // S->RightBranch[0].Top[Side_Left] = Side * 5.0f + S->Top[Side_Right];
                // S->RightBranch[0].Top[Side_Right] = Side * -5.0f + S->Top[Side_Right];

                // S->RightBranch[1].Top[Side_Left] = Side * 5.0f + Front * 10.0f + S->Top[Side_Right];
                // S->RightBranch[1].Top[Side_Right] = Side * -5.0f + Front * 10.0f + S->Top[Side_Right];
                // S->RightBranch[1].GrowRate[Side_Left] = 0.3f;
                // S->RightBranch[1].GrowRate[Side_Right] = 0.3f;

                // S->RightBranchCount = 2;
            }
        }

        if (S->LeafCount && S->Leaves[0].Size < 15.0f) {
            S->Leaves[0].Size += 0.5;
        }

        // if (S->LeftBranchCount) {
        //     GrowBranch(dT, S->LeftBranch, &S->LeftBranchCount, MaxRate / 10.0f, false);
        // }

        // if (S->RightBranchCount) {
        //     GrowBranch(dT, S->RightBranch, &S->RightBranchCount, MaxRate / 10.0f, false);
        // }

        S->Top[Side_Left] += LeftDirection * LeftGrowth;
        S->Top[Side_Right] += RightDirection * RightGrowth;
    }
}

struct edge {
    v2 A;
    v2 B;
    r32 dX;
    r32 X;
};

b32 NeedToSwap(edge E1, edge E2) { return Min_r32(E1.A.y, E1.B.y) > Min_r32(E2.A.y, E2.B.y); }

void
InsertSortEdges(edge *Edges, u32 Count)
{
    for (u32 i=1; i<Count; ++i) {
        s32 j=i-1;
        while (j >=0) {
            if (NeedToSwap(Edges[j], Edges[j + 1])) {
                edge Tmp = Edges[j];
                Edges[j] = Edges[j + 1];
                Edges[j + 1] = Tmp;
            }
            --j;
        }
    }
}

struct active_edge_array {
    edge *Edges;
    u32 Count;
    u32 Cap;

    void FitN(u32 N) {
        if (this->Count + N > this->Cap) {
            if (this->Edges) {
                u32 NewCap = this->Cap * 2;
                this->Edges = (edge *)realloc(this->Edges, NewCap);
                this->Cap = NewCap;
            } else {
                u32 Cap = Min_u32(N, 10);
                this->Edges = (edge *)malloc(Cap);
                this->Cap = Cap;
            }
        }
    }

    void Add(edge Edge) {
        this->FitN(1);

        s32 j = this->Count - 1;
        while (j >= 0) {
            if (Edges[j].X < Edges[j - 1].X) {
                edge Tmp = Edges[j];
                Edges[j] = Edges[j - 1];
                Edges[j - 1] = Tmp;
            }
            --j;
        }
    }

    b32 Remove(u32 Index, edge *Removed) {
        b32 Result = false;

        if (Index == this->Count - 1) {
            if (Removed) {
                *Removed = this->Edges[Index];
            }

            --this->Count;
            Result = true;
        } else {
            u32 MoveCount = this->Count - Index - 1;
            edge *To = this->Edges + Index;
            edge *From = To + 1;

            if (Removed) {
                *Removed = this->Edges[Index];
            }

            memmove(To, From, sizeof(edge) * MoveCount);

            --this->Count;
            Result = true;
        }

        return Result;
    }

    b32 Remove(edge *Edge, edge *Removed) {
        u32 Index = Edge - this->Edges;
        return this->Remove(Index, Removed);
    }
};

void plant::
ApplySunlight(v2 SunN, rect WorldAABB)
{
    v2 Center = WorldAABB.Min + WorldAABB.Dim() / 2.0f;
    r32 R = Length(WorldAABB.Min - Center);
    v2 SunP = (SunN * -1.0f) * R;

    v2 Y = SunN;
    v2 X = -1.0f * Perp(SunN);

    v2 *ScanlineEdges = (v2 *)malloc(sizeof(v2) * 1024);
    u32 ScanlineEdgeCount;

    edge *Edges = (edge *)malloc(sizeof(edge) * 1024);
    u32 EdgeCount = 0;
    u32 StartIndex = 0;

    active_edge_array ActiveEdges = {};

    for (u32 i=1; i < this->SegmentCount; ++i) {
        segment *S = this->Segments + i;
        segment *P = this->Segments + i - 1;

        v2 A = S->Top[Side_Left] - SunP;
        v2 B = S->Top[Side_Right] - SunP;
        v2 C = P->Top[Side_Right] - SunP;
        v2 D = P->Top[Side_Left] - SunP;

        A = X * A.x + Y * A.y;
        B = X * B.x + Y * B.y;
        C = X * C.x + Y * C.y;
        D = X * D.x + Y * D.y;

        Edges[EdgeCount + 0] = B.y > A.y ? edge{A, B} : edge{B, A};
        Edges[EdgeCount + 1] = C.y > B.y ? edge{B, C} : edge{C, B};
        Edges[EdgeCount + 2] = D.y > C.y ? edge{C, D} : edge{D, C};
        Edges[EdgeCount + 3] = A.y > D.y ? edge{D, A} : edge{A, D};

        lsfor(4) {
            edge *E = Edges + EdgeCount + i;
            E->dX = (E->B.y - E->A.y) / (E->B.x - E->A.x);
        }

        EdgeCount += 4;
    }

    InsertSortEdges(Edges, EdgeCount);

    r32 Scanline = 0.0f;

    while (true) {

        // add new edges to active edge array
        lsfor(EdgeCount - StartIndex) {
            edge *E = Edges + i;
            r32 Ex = E->A.x + (Scanline - E->A.y) * E->dX;

            lsforj(ScanlineEdgeCount) {
                v2 *SE = ScanlineEdges + j;
                if (Ex >= SE->a && Ex <= SE->b) {
                    // E->IntersectionX = Ex;
                    // ActiveEdges.push_back(*E);

                    break;
                }
            }
        }

        // walk over active edges to determine new shadowed areas

        b32 Inside = false;
        v2 Shadow = {};
        lsfor(ActiveEdges.Count) {
            edge *E = Edges + i;
            E->X += E->dX;

            if (Scanline > E->B.y) {
                // remove
            }

            if (Inside) {
                Shadow.b = E->X;

                lsforj(ScanlineEdgeCount) {
                    v2 *SE = ScanlineEdges + j;

                    b32 LeftInside = (SE->a >= Shadow.a && SE->a <= Shadow.b);
                    b32 RightInside = (SE->b >= Shadow.a && SE->b <= Shadow.b);

                    if (LeftInside && RightInside) {
                        // shadow is larger than the entire scanline segment. remove.
                    } else if (LeftInside) {
                        SE->a = Shadow.b;
                    } else if (RightInside) {
                        SE->b = Shadow.a;
                    } else if (SE->a >= Shadow.a && SE->b <= Shadow.b) {
                        // segment is bigger than shadow, and shadow is fully inside the segment
                        // so we have to subdivide the segment;

                        v2 S1 = { SE->a, Shadow.a };
                        v2 S2 = { Shadow.b, SE->b };
                    } else {
                        // segments dont intersect at all
                    }
                }
            } else {
                Shadow.a = E->X;
            }

            Inside = !Inside;
        }
    }
}

void
DrawPlant(segment *Segments, u32 SegmentCount, b32 Wireframe)
{
    for (u32 i=1; i < SegmentCount; ++i) {
        segment *S = Segments + i;
        segment *P = Segments + i - 1;

        if (Wireframe) {
            v2 Top[2] = { S->Top[Side_Left], S->Top[Side_Right] };
            v2 Right[2] = { S->Top[Side_Right], P->Top[Side_Right] };
            v2 Bottom[2] = { P->Top[Side_Left], P->Top[Side_Right] };
            v2 Left[2] = { P->Top[Side_Left], S->Top[Side_Left] };

            DrawLine(Top[0], Top[1], 2.0f, RGBA(136,167,151,200), 1);
            DrawLine(Right[0], Right[1], 2.0f, RGBA(136,167,151,200), 1);
            DrawLine(Bottom[0], Bottom[1], 2.0f, RGBA(136,167,151,200), 1);
            DrawLine(Left[0], Left[1], 2.0f, RGBA(136,167,151,200), 1);
        }

        if (S->LeafCount) {
            r32 Size = S->Leaves[0].Size;
            v2 Dir = S->Leaves[0].Direction;
            v2 Pos = S->Leaves[0].P;
            v2 P = Perp(S->Leaves[0].Direction);

            //
            v2 A = S->Leaves[0].P;
            v2 B = S->Leaves[0].P + Dir * (Size / 2.0f) + P * (Size / 2.0f);
            v2 Cunt = S->Leaves[0].P + Dir * (Size / 2.0f) - P * (Size / 2.0f);
            v2 D = S->Leaves[0].P + Dir*Size;

            DrawQuad(A, B, Cunt, D, v4{93,185,137,255}, 1);
            // DrawRect(v4{93,185,137,255}, S->Leaves[0].P, v2{, S->Leaves[0].Size}, 1);
        }

        // if (S->LeftBranchCount) {
        //     DrawPlant(S->LeftBranch, S->LeftBranchCount, true);
        // }

        // if (S->RightBranchCount) {
        //     DrawPlant(S->RightBranch, S->RightBranchCount, true);
        // }
    }
}

void
Cleanup(image Image)
{
    u32 Pitch = Image.N * Image.Width;
    for (u32 y=0; y<Image.Height; ++y) {
        for (u32 x=0; x<Image.Width; ++x) {
            u8 *Pixel = Image.Data + y * Pitch + x * Image.N;
            if (Pixel[0] == 0xff || Pixel[1] == 0xff || Pixel[2] == 0xff) {
                Pixel[0] = 0;
                Pixel[1] = 0;
                Pixel[2] = 0;
                Pixel[3] = 0;
            }
        }
    }
}

void
LoadImage(char *Path, image *Image)
{
    Image->Data = stbi_load(Path, (s32 *)&Image->Width, (s32 *)&Image->Height, (s32 *)&Image->N, 0);
    Image->Texture = OpenglUploadTexture(*Image);
}

void
GameInit()
{
    read_file FontFile;
    os::ReadFile("fonts.data", &FontFile);

    u8 *At = FontFile.Data;

    file_header Header = *(file_header *)At;
    u32 HeaderSize = sizeof(file_header);
    At += HeaderSize;

    for (u32 i=0; i<Header.Count; ++i) {
        cached_font *Font = FontCache + CachedFontCount;
        packed_font *Packed = (packed_font *)At;

        memcpy(Font->Name, Packed->Name, 20);
        Font->Id = Packed->Id;
        Font->GlyphCount = Packed->GlyphCount;
        Font->Atlas.Width = Packed->AtlasWidth;
        Font->Atlas.Height = Packed->AtlasHeight;
        Font->SizePx = Packed->SizePx;
        Font->PxPerFontUnit = Packed->PxPerFontUnit;
        Font->Height = Packed->Height;
        Font->Baseline = Packed->Baseline;
        Font->BaselineSpacing = Packed->BaselineSpacing;
        Font->Ascender = Packed->Ascender;
        Font->Descender = Packed->Descender;

        At += sizeof(packed_font);

        Font->Map = (uincode_character_map *)At;

        At += sizeof(uincode_character_map) * Font->GlyphCount;

        Font->Glyphs = (cached_glyph *)At;
        At += sizeof(cached_glyph) * Font->GlyphCount;

        Font->Advances = (r32 *)At;
        At += sizeof(r32) * Font->GlyphCount * Font->GlyphCount;

        char Name[30];
        sprintf(Name, "%s_%d.png", Font->Name, (u32)Font->SizePx);

        image Image = {};
        Image.Data = stbi_load(Name, (s32 *)&Image.Width, (s32 *)&Image.Height, (s32 *)&Image.N, 0);
        Font->Atlas.Texture = OpenglUploadTexture(Image);

        ++CachedFontCount;
    }

    // load texture
    // image Image = {};
    // Image.Data = stbi_load("'art'/main_module.png", (s32 *)&Image.Width, (s32 *)&Image.Height, (s32 *)&Image.N, 0);
    // Image.Texture = OpenglUploadTexture(Image);
}

void
OldPlant(r32 dT)
{
    if (Game.Steps < 500) {
        #define TRACK_FRAMES 20
        static r32 TrackedV[TRACK_FRAMES];
        static u32 CurrentV = 0;

        r32 MaxVelocityChange = 10.0f;

        v2 NextP = Game.TestPlant.P[Game.Steps];
        r32 Resistance = (NextP.x * NextP.x) * -1.0f * Sign_r32(NextP.x) / 70.0f;

        int Rand = rand() % 200;
        r32 Rand2 = ((r32)Rand - 100.0f) / 100.0f;

        NextP.y += 3;
        NextP.x += Game.Vx * dT;
        Game.Vx += Rand2 * MaxVelocityChange + (Resistance * dT);

        u32 VIndex = CurrentV % TRACK_FRAMES;
        TrackedV[VIndex] = Game.Vx;

        r32 Min = 9999.0f;
        r32 Max = -9999.0f;
        for (u32 i=0; i<TRACK_FRAMES; ++i) {
            Min = Min_r32(Min, TrackedV[i]);
            Max = Max_r32(Max, TrackedV[i]);
        }
        r32 DeltaV = Max - Min;

        ++CurrentV;
        ++Game.Steps;
        Game.TestPlant.P[Game.Steps] = NextP;
        Game.TestPlant.V[Game.Steps] = Game.Vx;
    }

    //
    //
    //

    for (u32 i=0; i<Game.Steps - 1; ++i) {
        r32 Red = 0.0;
        r32 Blue = 0.0;

        if (Game.TestPlant.V[i] < 0.0f) {
            Blue = -1.0f * Game.TestPlant.V[i] / 50.0f * 255.0f;
        } else {
            Red = Game.TestPlant.V[i] / 50.0f * 255.0f;
        }

        DrawLine(Game.TestPlant.P[i], Game.TestPlant.P[i+1], 4.0f, v4{Red, 0.0f, Blue, 255.0f}, 10);
        // DrawLine(Game.TestPlant[i].P, Game.TestPlant[i+1].P, 4.0f, v4{125.0f, 193.0f, 146.0f, 255.0f}, 10);
    }

}

void
DoGameFrame(r32 dT)
{
    r32 Scale;
    cached_font *Font = FindMatchingFont(Font_PTSans, 20.0f, &Scale);

    if (!Game.Initialized) {
        Game.Container = rect{v2{0.0f, 0.0f}, v2{500.0f, 300.0f}};
        Game.Soil = rect{v2{0.0f, 0.0f}, v2{500.0f, 50.0f}};

        v2 ContainerCenter = Game.Container.Min + (Game.Container.Max / 2.0f);
        Game.CameraP = ContainerCenter;

        Game.TestPlant.P[1].x = 0.0;
        Game.TestPlant.P[1].y = 1.0f;
        Game.Steps = 1;

        Game.Plant.Segments[0].Top[Side_Left] = {0.0f, 0.0f};
        Game.Plant.Segments[0].Top[Side_Right] = {10.0f, 0.0f};

        Game.Plant.Segments[1].Top[Side_Left] = {0.0f, 10.0f};
        Game.Plant.Segments[1].Top[Side_Right] = {10.0f, 10.0f};
        Game.Plant.Segments[1].GrowRate[Side_Left] = 1.0f;
        Game.Plant.Segments[1].GrowRate[Side_Right] = 1.0f;
        // Game.Plant.Segments[1].Parent = Game.Plant.Segments + 0;

        Game.Plant.SegmentCount = 2;

        Game.Initialized = true;
    }

    r32 CamSpeed = 500.0f;
    static v2 CamV = {};
    static r32 ZoomV = 0.0f;

    v3 FrameV = {};
    if (Input.Keys[Key_W].Down) {
        FrameV.y += 1.0f;
    }
    if (Input.Keys[Key_S].Down) {
        FrameV.y -= 1.0f;
    }
    if (Input.Keys[Key_A].Down) {
        FrameV.x -= 1.0f;
    }
    if (Input.Keys[Key_D].Down) {
        FrameV.x += 1.0f;
    }

    if (Input.MouseWheel != 0.0f) {
        FrameV.z = -Input.MouseWheel / 1000.0f;
        FrameV.z = Clamp(-3.0f, FrameV.z, 3.0f);
    }

    CamV += Normalize(FrameV.xy) * CamSpeed;
    CamV = CamV * 0.9f;

    ZoomV += FrameV.z * 15.0f;
    ZoomV = ZoomV * 0.9f;

    CamV.x = Clamp_r32(-CamSpeed, CamV.x, CamSpeed);
    CamV.y = Clamp_r32(-CamSpeed, CamV.y, CamSpeed);

    Game.CameraP += CamV * dT;
    Game.CameraScale += ZoomV * dT;

    //
    //
    //

    Game.Plant.GrowBranch(dT, Game.Plant.Segments, &Game.Plant.SegmentCount, Game.GrowRate);
    Game.Plant.ApplySunlight(v2{0.0, -1.0f}, rect{-100.0f, -100.0f, 500.0f, 500.0f});



    Game.GrowRate -= 0.0005;
    Game.GrowRate = Max(0.0f, Game.GrowRate);

    //
    //
    //

    Renderer.SetMatrix(Game.CameraP, true, true, Game.CameraScale);


    DrawRect(v4{28.0f, 21.0f, 33.0f, 255.0f}, Game.Soil.Min, Game.Soil.Dim(), 2);

    b32 Wireframe = true;
    DrawPlant(Game.Plant.Segments, Game.Plant.SegmentCount, true);

    DrawCircle(RGBA(213,158,79,255), Game.SunP, 30.0f, 1);

    Renderer.Flush();

    // Renderer.SetMatrix(v2{0.0f, 0.0f}, false, false, 1.0f);
    // DrawRect(v4{255.0f,10.0f,10.0f,255.0f}, v2{0.0f, 0.0f}, v2{100.0f, 100.0f}, 0.0f);
    // DrawText(v2{0.0f, 0.0f}, 100, v4{100.0f, 200.0f, 100.0f, 255.0f}, Font_PTSans, 12.0f, "Success!");
    // Renderer.Flush();

    // v2 WorldMouseP = Input.MouseP - v2{(r32)Render.Screen.x / 2.0f, (r32)Render.Screen.y / 2.0f};
    // WorldMouseP += Render.CameraP / Render.CameraScale;

    Game.Time += dT;
}