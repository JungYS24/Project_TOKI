# 🌑 Project TOKI - 토벌대 키우기

![Generic badge](https://img.shields.io/badge/Engine-Unreal%20Engine%205.6-black?logo=unrealengine)
![Generic badge](https://img.shields.io/badge/Platform-Android%20%7C%20iOS-green?logo=android)
![Generic badge](https://img.shields.io/badge/Genre-Idle%20RPG-blueviolet)
![Generic badge](https://img.shields.io/badge/Status-Prototyping-yellow)

> **"악몽을 먹어치우는 자들의 이야기"**
>
> 서브컬처 다크 판타지 감성과 고효율 로우폴리 3D가 결합된 모바일 방치형 RPG 프로젝트입니다.

---

## 🎨 Visual Identity

**"No Pixel, Just Art."**
양산형 도트 그래픽을 배제하고, 고유의 아트웍과 최적화된 3D 기술로 승부합니다.

### 🖌️ 2D & UI (Emotion)
* **Original Artwork:** 102종 이상의 자체 제작 고퀄리티 캐릭터 일러스트 보유.
* **Deep Dark UI:** 블랙/투명 그라데이션을 활용한 'Extreme Minimal' 인터페이스.
* **Cut-In System:** 스킬 발동 시 일러스트가 화면을 찢고 나오는 역동적 연출.

### 🧊 3D Character (Action)
* **Style:** SD(Super Deformed) 비율의 로우폴리 캐릭터.
* **Optimization:**
    * **Single Texture Strategy:** 노멀/러프니스 맵을 배제하고 `Base Color` 1장으로 디테일 표현 (Hand-Painted).
    * **Low Poly:** 캐릭터당 4,000 Triangles 미만, 몬스터 1,500 Triangles 미만 유지.
    * **A-Pose:** 언리얼 5 마네킹 호환성을 고려한 A-Pose 리깅.

---

## 🛠️ Technical Specs

### Core System
* **View:** 9:16 Portrait Mode (세로형) / Quarter View (45°).
* **Screen Ratio:** 80% (3D Battle World) : 20% (UI Dock).
* **Map Design:** 디오라마(Diorama) 스타일의 공중 부유형 스테이지 (배경 리소스 최소화).

### Development Environment
* **Engine:** Unreal Engine 5 (Blueprint + Python Automation).
* **AI Assistant:** **MCP (Model Context Protocol)** 기반 개발 파이프라인 구축.
    * *AI(Antigravity)가 언리얼 에디터를 직접 제어하여 레벨 디자인 및 노드 구성을 자동화.*

---

## 📂 Directory Structure

```text
/Content/Project_TOKI
├── 00_Blueprints   # Core Logic (GameMode, Controller, AI)
├── 01_Maps         # Levels (Diorama Stage)
├── 02_Data         # Data Tables
├── 03_Characters   # 3D Assets & Anim
├── 04_UI           # UMG Widgets
├── 05_Art          # 2D Illustrations
└── 06_Sound        # BGM & SFX
