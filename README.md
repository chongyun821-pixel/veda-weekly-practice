# veda-weekly-practice

티스토리 블로그에 임베드할 인터랙티브 학습 예제를 GitHub Pages로 배포하기 위한 정적 사이트입니다.

배포 주소:

- `https://chongyun821-pixel.github.io/veda-weekly-practice/`
- `https://chongyun821-pixel.github.io/veda-weekly-practice/pages/week1/problems/problems.html`

현재 포함된 예제:

- Week 1: C++ 클래스 스택 구현 빈칸 채우기
- Week 1: 버블 정렬 빈칸 채우기

## 폴더 구조

```text
.
├── index.html
└── pages
    └── week1
        └── problems
            └── problems.html
```

## GitHub Pages 배포 방법

1. GitHub에서 `veda-weekly-practice` 리포지토리를 생성합니다.
2. 이 폴더의 파일을 리포지토리 루트에 업로드합니다.
3. GitHub 저장소의 `Settings > Pages`로 이동합니다.
4. `Build and deployment`에서 `Deploy from a branch`를 선택합니다.
5. 브랜치는 `main`, 폴더는 `/ (root)`를 선택하고 저장합니다.
6. 배포가 끝나면 아래 주소로 접속합니다.

```text
https://chongyun821-pixel.github.io/veda-weekly-practice/
```

## 티스토리 임베드 예시

티스토리 글쓰기 화면에서 HTML 모드로 전환한 뒤 아래 코드를 넣으면 됩니다.

```html
<iframe
  src="https://chongyun821-pixel.github.io/veda-weekly-practice/pages/week1/problems/problems.html"
  width="100%"
  height="1500"
  style="border:0; border-radius:16px; overflow:hidden;"
  loading="lazy"
  allowfullscreen
></iframe>
```

## 메모

- 티스토리 본문에서는 자바스크립트 렌더링 제약이 있을 수 있으므로, 인터랙티브 로직은 GitHub Pages 쪽에서 처리합니다.
- 이후 주차가 늘어나면 `pages/week2`, `pages/week3` 같은 식으로 확장하면 됩니다.
