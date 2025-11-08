#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <cmath>
#include <glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>   // 꼭 추가!

struct DrawRange {
    GLsizei first = 0; // 시작 '정점' 인덱스
    GLsizei count = 0; // 정점 개수 (삼각형 개수 * 3)
};

struct CubeMesh {
    GLuint vao = 0, vbo = 0;
    GLsizei count = 0;                  // 전체 정점 수
    std::vector<DrawRange> faceRanges;  // +X,-X,+Y,-Y,+Z,-Z 순서로 최대 6개 (비어있으면 생략)
};

// ---------- 유틸 ----------
static inline void RemoveUTF8BOM(std::string& s) {
    if (s.size() >= 3 &&
        (unsigned char)s[0] == 0xEF &&
        (unsigned char)s[1] == 0xBB &&
        (unsigned char)s[2] == 0xBF) {
        s.erase(0, 3);
    }
}

struct Idx { int v = 0, vt = 0, vn = 0; };

// "v/vt/vn", "v//vn", "v/vt", "v"
static inline Idx ParseFaceToken(const std::string& tok) {
    Idx out{};
    int slash1 = (int)tok.find('/');
    if (slash1 == (int)std::string::npos) {
        out.v = std::stoi(tok);
        return out;
    }
    int slash2 = (int)tok.find('/', slash1 + 1);
    out.v = std::stoi(tok.substr(0, slash1));
    if (slash2 == (int)std::string::npos) {
        // v/vt
        std::string vt = tok.substr(slash1 + 1);
        if (!vt.empty()) out.vt = std::stoi(vt);
    }
    else {
        // v/vt/vn 또는 v//vn
        std::string vt = tok.substr(slash1 + 1, slash2 - slash1 - 1);
        std::string vn = tok.substr(slash2 + 1);
        if (!vt.empty()) out.vt = std::stoi(vt);
        if (!vn.empty()) out.vn = std::stoi(vn);
    }
    return out;
}

// OBJ 인덱스 보정(1-based, 음수 인덱스 허용)
static inline int FixIndex(int idx, int n) {
    if (idx > 0) return idx;            // 1..n
    if (idx < 0) return n + idx + 1;    // -1 => n, -2 => n-1 ...
    return 0;                           // 0은 무효
}

// ---------- 로더 (B 방식: 버킷팅) ----------
static bool LoadOBJ_PosNorm_Interleaved(const char* path, CubeMesh& out)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "[OBJ] failed to open: " << path << "\n";
        return false;
    }

    std::vector<glm::vec3> positions;  // 1-based
    std::vector<glm::vec3> normals;    // 1-based

    // 6개 면 버킷 (+X,-X,+Y,-Y,+Z,-Z)
    std::vector<float> buckets[6];

    auto pushV = [](std::vector<float>& dst, const glm::vec3& P, const glm::vec3& N) {
        dst.push_back(P.x); dst.push_back(P.y); dst.push_back(P.z);
        dst.push_back(N.x); dst.push_back(N.y); dst.push_back(N.z);
        };

    // 면 법선 -> 버킷 인덱스
    auto faceBucket = [](const glm::vec3& n) -> int {
        glm::vec3 fn = glm::normalize(n);
        float ax = std::fabs(fn.x), ay = std::fabs(fn.y), az = std::fabs(fn.z);
        int axis; // 0:x,1:y,2:z
        if (ax >= ay && ax >= az) axis = 0;
        else if (ay >= az)        axis = 1;
        else                      axis = 2;

        bool positive = (axis == 0 ? fn.x >= 0.0f
            : axis == 1 ? fn.y >= 0.0f
            : fn.z >= 0.0f);
        // +X=0,-X=1,+Y=2,-Y=3,+Z=4,-Z=5
        static const int map[3][2] = { {0,1}, {2,3}, {4,5} };
        return map[axis][positive ? 0 : 1];
        };

    std::string line;
    bool firstLine = true;

    // 삼각형 하나 push (버킷까지)
    auto emitTriangle = [&](const Idx& a, const Idx& b, const Idx& c) {
        int va = FixIndex(a.v, (int)positions.size());
        int vb = FixIndex(b.v, (int)positions.size());
        int vc = FixIndex(c.v, (int)positions.size());
        int na = FixIndex(a.vn, (int)normals.size());
        int nb = FixIndex(b.vn, (int)normals.size());
        int nc = FixIndex(c.vn, (int)normals.size());
        if (va <= 0 || vb <= 0 || vc <= 0) return;

        glm::vec3 Pa = positions[va - 1];
        glm::vec3 Pb = positions[vb - 1];
        glm::vec3 Pc = positions[vc - 1];

        glm::vec3 Na, Nb, Nc;
        if (na > 0 && nb > 0 && nc > 0) {
            Na = glm::normalize(normals[na - 1]);
            Nb = glm::normalize(normals[nb - 1]);
            Nc = glm::normalize(normals[nc - 1]);
        }
        else {
            // vn 없으면 face normal 사용(정점 동일)
            glm::vec3 fn = glm::normalize(glm::cross(Pb - Pa, Pc - Pa));
            Na = Nb = Nc = fn;
        }

        // 삼각형 face normal
        glm::vec3 fN = glm::normalize(glm::cross(Pb - Pa, Pc - Pa));
        if (!std::isfinite(fN.x) || !std::isfinite(fN.y) || !std::isfinite(fN.z))
            return;

        // 어떤 면 버킷인가?
        int bkt = faceBucket(fN);

        // ★ 버킷의 목표 법선(+X,-X,+Y,-Y,+Z,-Z)
        static const glm::vec3 targetN[6] = {
            {+1,0,0}, {-1,0,0}, {0,+1,0}, {0,-1,0}, {0,0,+1}, {0,0,-1}
        };
        glm::vec3 tn = targetN[bkt];

        // ★ 목표 방향과 반대면 와인딩 뒤집기 (b,c swap)
        if (glm::dot(fN, tn) < 0.0f) {
            std::swap(Pb, Pc);
            std::swap(Nb, Nc);
            fN = -fN; // 선택적 업데이트
        }

        // 버킷에 push (정점 노말은 원래대로/혹은 fn으로, 둘 다 OK)
        pushV(buckets[bkt], Pa, Na);
        pushV(buckets[bkt], Pb, Nb);
        pushV(buckets[bkt], Pc, Nc);
        };

    while (std::getline(ifs, line)) {
        if (firstLine) { RemoveUTF8BOM(line); firstLine = false; }
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        // 좌측 공백 제거
        size_t s = 0; while (s < line.size() && std::isspace((unsigned char)line[s])) ++s;
        if (s >= line.size()) continue;

        std::istringstream iss(line.substr(s));
        std::string tag; iss >> tag;

        if (tag == "v") {
            glm::vec3 p; iss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        }
        else if (tag == "vn") {
            glm::vec3 n; iss >> n.x >> n.y >> n.z;
            normals.push_back(glm::normalize(n));
        }
        else if (tag == "f") {
            // 폴리곤 -> triangle fan
            std::vector<Idx> vs;
            std::string tok;
            while (iss >> tok) vs.push_back(ParseFaceToken(tok));
            if (vs.size() < 3) continue;
            for (size_t i = 2; i < vs.size(); ++i) {
                emitTriangle(vs[0], vs[i - 1], vs[i]);
            }
        }
        // (vt, usemtl, mtllib, o, g, s ...)은 무시
    }

    // ----- 6개 버킷을 한 VBO로 이어붙이면서 faceRanges 기록 -----
    std::vector<float> interleaved;
    interleaved.reserve(1 << 16);
    out.faceRanges.clear();

    for (int b = 0; b < 6; ++b) {
        if (buckets[b].empty()) continue;
        GLsizei start = static_cast<GLsizei>(interleaved.size() / 6);
        interleaved.insert(interleaved.end(), buckets[b].begin(), buckets[b].end());
        GLsizei cnt = static_cast<GLsizei>(buckets[b].size() / 6);
        out.faceRanges.push_back({ start, cnt });
    }

    // GL 버퍼 업로드
    glGenVertexArrays(1, &out.vao);
    glGenBuffers(1, &out.vbo);

    glBindVertexArray(out.vao);
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
    glBufferData(GL_ARRAY_BUFFER,
        interleaved.size() * sizeof(float),
        interleaved.data(),
        GL_STATIC_DRAW);

    // layout(location=0) vec3 aPos;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);
    glEnableVertexAttribArray(0);
    // layout(location=1) vec3 aNormal;
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    out.count = static_cast<GLsizei>(interleaved.size() / 6);
    if (out.count == 0) {
        std::cerr << "[OBJ] no triangles parsed or all degenerate: " << path << "\n";
        return false;
    }
    return true;
}