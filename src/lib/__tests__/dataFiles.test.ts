import { describe, it, expect } from 'vitest';
import { readFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import yaml from 'js-yaml';
import { isProgrammeGroup } from '$lib/compareFilters';
import { versionProgrammes } from '$lib/programmes';
import { buildSchemeIndex } from '$lib/slugs';
import { isHistoryType } from '$lib/timeline';
import type { HistoryEntry, KemSchemeYaml, ProgrammeYaml, SchemeYaml } from '$lib/types';

const DATA = join(process.cwd(), 'data');
const load = <T>(path: string): T => yaml.load(readFileSync(path, 'utf8')) as T;

const programmes = load<ProgrammeYaml[]>(join(DATA, 'programmes.yaml'));
const stagesOf = new Map(programmes.map((p) => [p.id, new Set(p.stages.map((s) => s.id))]));

function checkMembership(file: string, map: Record<string, string | string[]> | undefined): string[] {
	return Object.entries(map ?? {}).flatMap(([programme, value]) => {
		const stages = stagesOf.get(programme);
		if (!stages) return [`${file}: unknown programme "${programme}"`];
		return [value]
			.flat()
			.flatMap((stage) => (stages.has(stage) ? [] : [`${file}: unknown stage "${stage}" in programme "${programme}"`]));
	});
}

describe('data/programmes.yaml', () => {
	it('has unique programme ids', () => {
		const ids = programmes.map((p) => p.id);
		expect(new Set(ids).size).toBe(ids.length);
	});
	it('has unique stage ids within each programme', () => {
		for (const p of programmes) {
			const ids = p.stages.map((s) => s.id);
			expect(new Set(ids).size, p.id).toBe(ids.length);
		}
	});

	it('maps every programme onto a compare-page filter group', () => {
		// nist-pqc splits into nist-standards / nist-pqc-rounds; every other id is its own group.
		const unmapped = programmes.map((p) => p.id).filter((id) => id !== 'nist-pqc' && !isProgrammeGroup(id));
		expect(unmapped).toEqual([]);
	});
});

describe('programme references in scheme data', () => {
	it('no signature version still uses legacy tags', () => {
		const dir = join(DATA, 'schemes');
		const offenders = readdirSync(dir)
			.filter((f) => f.endsWith('.yaml'))
			.filter((f) => load<{ versions: Record<string, unknown>[] }>(join(dir, f)).versions.some((v) => 'tags' in v));
		expect(offenders).toEqual([]);
	});
	it('the on-ramp has 40, 14 and 9 schemes in rounds 1, 2 and 3', () => {
		const dir = join(DATA, 'schemes');
		const count = (stage: string) =>
			readdirSync(dir)
				.filter((f) => f.endsWith('.yaml'))
				.filter((f) =>
					load<SchemeYaml>(join(dir, f)).versions.some((v) => versionProgrammes(v)['nist-onramp']?.includes(stage))
				).length;
		expect([count('round-1'), count('round-2'), count('round-3')]).toEqual([40, 14, 9]);
	});
	it('China NGCC round 1 has 34 signature and 41 KEM files', () => {
		const sigs = readdirSync(join(DATA, 'schemes'))
			.filter((f) => f.endsWith('.yaml'))
			.filter((f) =>
				load<SchemeYaml>(join(DATA, 'schemes', f)).versions.some((v) =>
					versionProgrammes(v)['china-ngcc']?.includes('round-1')
				)
			).length;
		const kems = readdirSync(join(DATA, 'kems'))
			.filter((f) => f.endsWith('.yaml'))
			.filter((f) =>
				[load<KemSchemeYaml>(join(DATA, 'kems', f)).programmes?.['china-ngcc'] ?? []].flat().includes('round-1')
			).length;
		expect([sigs, kems]).toEqual([34, 41]);
	});
	it('every signature version references a known programme and stage', () => {
		const dir = join(DATA, 'schemes');
		const errors = readdirSync(dir)
			.filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => load<SchemeYaml>(join(dir, f)).versions.flatMap((v) => checkMembership(f, v.programmes)));
		expect(errors).toEqual([]);
	});
	it('every KEM references a known programme and stage', () => {
		const dir = join(DATA, 'kems');
		const errors = readdirSync(dir)
			.filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => checkMembership(f, load<KemSchemeYaml>(join(dir, f)).programmes));
		expect(errors).toEqual([]);
	});
	it('every membership is in a programme that covers that primitive', () => {
		const primitivesOf = new Map(programmes.map((p) => [p.id, p.primitives]));
		const wrong = (file: string, ids: string[], primitive: 'signature' | 'kem') =>
			ids
				.filter((id) => primitivesOf.has(id) && !primitivesOf.get(id)!.includes(primitive))
				.map((id) => `${file}: programme "${id}" does not cover ${primitive}s`);
		const sigErrors = readdirSync(join(DATA, 'schemes'))
			.filter((f) => f.endsWith('.yaml'))
			.flatMap((f) =>
				wrong(
					f,
					[
						...new Set(
							load<SchemeYaml>(join(DATA, 'schemes', f)).versions.flatMap((v) => Object.keys(versionProgrammes(v)))
						)
					],
					'signature'
				)
			);
		const kemErrors = readdirSync(join(DATA, 'kems'))
			.filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => wrong(f, Object.keys(versionProgrammes(load<KemSchemeYaml>(join(DATA, 'kems', f)))), 'kem'));
		expect([...sigErrors, ...kemErrors]).toEqual([]);
	});
});

describe('security levels in data', () => {
	const NGCC = new Set([128, 256, 384, 512]);
	type Ps = { name: string; level?: unknown; ngcc_level?: unknown };
	function check(file: string, sets: Ps[]): string[] {
		return sets.flatMap((ps) => {
			const hasNist = ps.level !== undefined;
			const hasNgcc = ps.ngcc_level !== undefined;
			if (hasNist === hasNgcc) return [`${file} ${ps.name}: needs exactly one of level / ngcc_level`];
			if (hasNgcc && !NGCC.has(ps.ngcc_level as number))
				return [`${file} ${ps.name}: ngcc_level must be 128, 256, 384 or 512`];
			return [];
		});
	}
	it('every signature parameter set states exactly one level scale', () => {
		const dir = join(DATA, 'schemes');
		const errors = readdirSync(dir)
			.filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => load<SchemeYaml>(join(dir, f)).versions.flatMap((v) => check(f, v.parametersets as Ps[])));
		expect(errors).toEqual([]);
	});
	it('every KEM parameter set states exactly one level scale', () => {
		const dir = join(DATA, 'kems');
		const errors = readdirSync(dir)
			.filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => check(f, load<KemSchemeYaml>(join(dir, f)).parametersets as Ps[]));
		expect(errors).toEqual([]);
	});
});

describe('scheme pages', () => {
	const readDir = <T>(dir: string): T[] =>
		readdirSync(join(DATA, dir))
			.filter((f) => f.endsWith('.yaml'))
			.map((f) => load<T>(join(DATA, dir, f)));
	const sigs = readDir<SchemeYaml>('schemes');
	const kems = readDir<KemSchemeYaml>('kems');

	it('gives every signature and KEM a unique lowercase ASCII slug and a unique name', () => {
		// On failure the thrown message names both schemes; add `slug:` to one YAML file.
		expect(() => buildSchemeIndex(sigs, kems)).not.toThrow();
	});

	it('every history entry names a scheme or KEM that has a page', () => {
		const names = new Set([...sigs, ...kems].map((s) => s.name));
		const history = load<HistoryEntry[]>(join(DATA, 'history.yaml'));
		const unknown = history.flatMap((e) =>
			(e.schemes ?? []).filter((n) => !names.has(n)).map((n) => `${e.date}: "${n}"`)
		);
		expect(unknown).toEqual([]);
	});
});

describe('data/history.yaml', () => {
	type Entry = { date?: unknown; type?: unknown; description?: unknown; schemes?: unknown; programmes?: unknown };

	// Scheme names are checked by 'every history entry names a scheme or KEM that has a page' (scheme pages block).
	/** One message per problem, naming the entry by position and date. */
	function historyErrors(entries: Entry[], programmeIds: Set<string>): string[] {
		return entries.flatMap((e, i) => {
			const where = `history.yaml entry ${i + 1} (${String(e.date)})`;
			const errors: string[] = [];
			if (typeof e.date !== 'string' || !/^\d{4}-\d{2}-\d{2}$/.test(e.date)) {
				errors.push(`${where}: date must be a quoted 'YYYY-MM-DD' string`);
			}
			if (!isHistoryType(e.type)) errors.push(`${where}: unknown type "${String(e.type)}"`);
			if (typeof e.description !== 'string' || e.description.trim() === '') errors.push(`${where}: empty description`);
			if (e.schemes !== undefined && (!Array.isArray(e.schemes) || e.schemes.length === 0)) {
				errors.push(`${where}: schemes must be a non-empty list`);
			}
			if (e.programmes !== undefined) {
				if (!Array.isArray(e.programmes) || e.programmes.length === 0) {
					errors.push(`${where}: programmes must be a non-empty list`);
				} else {
					for (const v of e.programmes) if (!programmeIds.has(v)) errors.push(`${where}: unknown programme "${v}"`);
				}
			}
			return errors;
		});
	}

	const programmeIds = new Set(programmes.map((p) => p.id));

	it('every entry is well formed and names only known programmes', () => {
		const history = load<Entry[]>(join(DATA, 'history.yaml'));
		expect(history.length).toBeGreaterThan(0);
		expect(historyErrors(history, programmeIds)).toEqual([]);
	});

	it('reports an unknown programme, naming the entry', () => {
		const bad: Entry[] = [{ date: '2026-01-02', type: 'attack', description: 'x', programmes: ['ngcc'] }];
		expect(historyErrors(bad, new Set(['kpqc']))).toEqual([
			'history.yaml entry 1 (2026-01-02): unknown programme "ngcc"'
		]);
	});

	it('rejects an unquoted date (js-yaml reads it as a Date)', () => {
		const parsed = yaml.load('- date: 2026-01-02\n  type: update\n  description: x\n') as Entry[];
		expect(historyErrors(parsed, programmeIds)[0]).toMatch(/entry 1 .*date must be a quoted/);
	});

	it('rejects an unknown type, an empty description and an empty list', () => {
		const bad: Entry[] = [{ date: '2026-01-02', type: 'standardisation', description: ' ', schemes: [] }];
		expect(historyErrors(bad, programmeIds)).toEqual([
			'history.yaml entry 1 (2026-01-02): unknown type "standardisation"',
			'history.yaml entry 1 (2026-01-02): empty description',
			'history.yaml entry 1 (2026-01-02): schemes must be a non-empty list'
		]);
	});
});
