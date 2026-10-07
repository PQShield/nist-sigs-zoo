import { describe, it, expect } from 'vitest';
import { readFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import yaml from 'js-yaml';
import { versionProgrammes } from '$lib/programmes';
import type { KemSchemeYaml, ProgrammeYaml, SchemeYaml } from '$lib/types';

const DATA = join(process.cwd(), 'data');
const load = <T>(path: string): T => yaml.load(readFileSync(path, 'utf8')) as T;

const programmes = load<ProgrammeYaml[]>(join(DATA, 'programmes.yaml'));
const stagesOf = new Map(programmes.map((p) => [p.id, new Set(p.stages.map((s) => s.id))]));

function checkMembership(file: string, map: Record<string, string | string[]> | undefined): string[] {
	return Object.entries(map ?? {}).flatMap(([programme, value]) => {
		const stages = stagesOf.get(programme);
		if (!stages) return [`${file}: unknown programme "${programme}"`];
		return [value].flat().flatMap((stage) =>
			stages.has(stage) ? [] : [`${file}: unknown stage "${stage}" in programme "${programme}"`]
		);
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
				.filter((f) => load<SchemeYaml>(join(dir, f)).versions.some((v) => versionProgrammes(v)['nist-onramp']?.includes(stage))).length;
		expect([count('round-1'), count('round-2'), count('round-3')]).toEqual([40, 14, 9]);
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
});

describe('security levels in data', () => {
	const NGCC = new Set([128, 256, 384, 512]);
	type Ps = { name: string; level?: unknown; ngcc_level?: unknown };
	function check(file: string, sets: Ps[]): string[] {
		return sets.flatMap((ps) => {
			const hasNist = ps.level !== undefined;
			const hasNgcc = ps.ngcc_level !== undefined;
			if (hasNist === hasNgcc) return [`${file} ${ps.name}: needs exactly one of level / ngcc_level`];
			if (hasNgcc && !NGCC.has(ps.ngcc_level as number)) return [`${file} ${ps.name}: ngcc_level must be 128, 256, 384 or 512`];
			return [];
		});
	}
	it('every signature parameter set states exactly one level scale', () => {
		const dir = join(DATA, 'schemes');
		const errors = readdirSync(dir).filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => load<SchemeYaml>(join(dir, f)).versions.flatMap((v) => check(f, v.parametersets as Ps[])));
		expect(errors).toEqual([]);
	});
	it('every KEM parameter set states exactly one level scale', () => {
		const dir = join(DATA, 'kems');
		const errors = readdirSync(dir).filter((f) => f.endsWith('.yaml'))
			.flatMap((f) => check(f, load<KemSchemeYaml>(join(dir, f)).parametersets as Ps[]));
		expect(errors).toEqual([]);
	});
});
