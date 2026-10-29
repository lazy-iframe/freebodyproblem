// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

// Served by GitHub Pages as a project site: lazy-iframe.github.io/freebodyproblem.
// Every internal link and asset is prefixed with `base`, so moving to a custom
// domain is a matter of changing these two lines (and dropping `base`).
export default defineConfig({
	site: 'https://lazy-iframe.github.io',
	base: '/freebodyproblem',
	trailingSlash: 'always',

	// The screenshots live in the repository's screens/, shared with the README,
	// and the pages import them from there. The dev server has to be allowed to
	// read outside site/ for that; the build needs nothing.
	vite: { server: { fs: { allow: ['..'] } } },

	integrations: [
		starlight({
			title: 'Free Body Problem',
			description:
				'A fast ground control station for ArduPilot and PX4: several links and several vehicles at once.',
			favicon: '/favicon.png',
			// Dark only: no light theme, and no switch for one. The landing
			// page's top is our own — see src/components/Hero.astro.
			components: {
				Hero: './src/components/Hero.astro',
				ThemeProvider: './src/components/ThemeProvider.astro',
				ThemeSelect: './src/components/ThemeSelect.astro',
			},
			social: [
				{ icon: 'github', label: 'GitHub', href: 'https://github.com/lazy-iframe/freebodyproblem' },
			],
			editLink: {
				baseUrl: 'https://github.com/lazy-iframe/freebodyproblem/edit/main/site/',
			},
			customCss: [
				'@fontsource-variable/geist',
				'@fontsource-variable/geist-mono',
				'./src/styles/theme.css',
			],
			sidebar: [
				{
					label: 'Getting started',
					items: [
						{ label: 'Install', slug: 'start/install' },
						{ label: 'Quick start', slug: 'start/quick-start' },
						{ label: 'Building from source', slug: 'start/building' },
					],
				},
				{
					label: 'Using it',
					items: [
						'guides/flight',
						'guides/map',
						'guides/mission',
						'guides/parameters',
						'guides/swarm',
						'guides/radio',
						'guides/sensors',
						'guides/video',
						'guides/mavlink',
						'guides/audio',
						'guides/themes',
						'guides/agents',
						'guides/screen-sizes',
					],
				},
				{
					label: 'Reference',
					items: ['reference/architecture', 'reference/configuration', 'reference/limitations'],
				},
			],
		}),
	],
});
