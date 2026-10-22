# Free Body Problem — website

The project site and documentation, built with [Astro](https://astro.build) and
[Starlight](https://starlight.astro.build), published to GitHub Pages at
<https://lazy-iframe.github.io/freebodyproblem/> by `.github/workflows/site.yml`.

```bash
npm ci            # once
npm run dev       # http://localhost:4321/freebodyproblem/
npm run build     # static site in dist/
```

- Pages are Markdown in `src/content/docs/`; the sidebar is in `astro.config.mjs`.
- Screenshots are imported from the repository's `screens/`, shared with the
  README, and optimised to WebP at build time.
- The download buttons are resolved at build time from the latest GitHub
  release (`src/lib/release.ts`). CD redeploys the site after each release.
