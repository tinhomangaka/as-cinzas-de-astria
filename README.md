# As Cinzas de Astria — Android v0.4

A v0.4 reorganiza o projeto para seguir a linha de build Android usada pelo ecossistema oficial do raylib, em vez de depender de uma Activity Java genérica para hospedar a janela do jogo.

A documentação e os templates oficiais do raylib mantêm um fluxo específico para Android, incluindo `Makefile.Android`, NDK e arquitetura ARM64. O workflow deste projeto usa esse fluxo em uma máquina GitHub Actions.

## Novidades

- identidade visual pixel-art procedural;
- animação simples dos personagens;
- Vila Aurora;
- Floresta Cinzenta;
- Santuário;
- NPCs;
- diálogos;
- diário de missões;
- transição entre mapas;
- encontros aleatórios;
- três tipos de inimigos;
- chefe Guardião de Cinzas;
- ataque, magia, item e fuga;
- XP e níveis;
- ouro;
- loja;
- arma e armadura;
- tela de grupo;
- save/load;
- controles touch;
- joystick virtual;
- controles por teclado;
- build automatizado no GitHub Actions.

## Build pelo GitHub

1. Crie um repositório no GitHub.
2. Envie todo o conteúdo deste projeto.
3. Abra a aba **Actions**.
4. Execute **Build Android APK — As Cinzas de Astria v0.4**.
5. Ao terminar, abra a execução e baixe o artefato `astria-v04-apk`.

O workflow usa o template de jogo Android do raylib como base de compilação. O projeto do jogo continua sendo o código original em `src/astria.c`.

## Build local

O método oficial do raylib para Android depende do Android SDK/NDK e de ferramentas de build. No celular, o build remoto pelo GitHub Actions tende a ser muito mais simples.

## Controles

### Mundo
- joystick virtual: movimento
- A: interagir
- M: menu

### Teclado
- WASD/setas: movimento
- E: interagir
- TAB: menu
- F5: salvar

### Batalha
- 1: ataque
- 2: magia
- 3: item
- 4: fuga

## Estrutura

```
AsCinzasDeAstria/
├── .github/workflows/android.yml
├── src/astria.c
├── README.md
└── LICENSE.txt
```

O jogo é original e não utiliza personagens, mapas, sprites ou músicas de Final Fantasy VI.
