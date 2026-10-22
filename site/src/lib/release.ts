// The latest release's downloads, looked up once per build.
//
// The installers carry their version in the file name, so no fixed URL stays
// pointed at the newest one. The site asks the GitHub API at build time instead,
// and the CD workflow rebuilds the site after every release, so the buttons move
// with it. "latest" is GitHub's: the newest release that is not a prerelease.
//
// A failed lookup (offline, rate-limited) does not fail the build: the buttons
// fall back to the latest-release page, where both files are one click away.

const REPO = 'lazy-iframe/freebodyproblem';
export const RELEASES_URL = `https://github.com/${REPO}/releases`;
export const LATEST_URL = `${RELEASES_URL}/latest`;

export interface Download {
	url: string;
	name: string | null; // asset file name, null on the fallback
	size: number | null; // bytes
}

export interface LatestRelease {
	tag: string | null;
	page: string;
	windows: Download;
	linux: Download;
}

const fallback = (): Download => ({ url: LATEST_URL, name: null, size: null });

let cached: Promise<LatestRelease> | undefined;

export function latestRelease(): Promise<LatestRelease> {
	cached ??= lookup();
	return cached;
}

async function lookup(): Promise<LatestRelease> {
	const result: LatestRelease = { tag: null, page: LATEST_URL, windows: fallback(), linux: fallback() };
	try {
		const headers: Record<string, string> = { Accept: 'application/vnd.github+json' };
		// In Actions the workflow's token lifts the anonymous rate limit.
		const token = process.env.GITHUB_TOKEN;
		if (token) headers.Authorization = `Bearer ${token}`;

		const res = await fetch(`https://api.github.com/repos/${REPO}/releases/latest`, { headers });
		if (!res.ok) throw new Error(`GitHub API ${res.status}`);
		const rel = await res.json();

		result.tag = rel.tag_name ?? null;
		result.page = rel.html_url ?? LATEST_URL;
		for (const a of rel.assets ?? []) {
			const d: Download = { url: a.browser_download_url, name: a.name, size: a.size };
			if (/\.exe$/i.test(a.name)) result.windows = d;
			else if (/\.deb$/i.test(a.name)) result.linux = d;
		}
	} catch (err) {
		console.warn(`[release] latest release lookup failed, linking the releases page: ${err}`);
	}
	return result;
}

export function formatSize(bytes: number | null): string {
	if (!bytes) return '';
	return bytes >= 1024 * 1024 ? `${(bytes / 1024 / 1024).toFixed(0)} MB` : `${(bytes / 1024).toFixed(0)} KB`;
}
