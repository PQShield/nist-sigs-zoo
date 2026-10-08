import { describe, it, expect } from 'vitest';
import { existsSync, readFileSync } from 'node:fs';
import { join } from 'node:path';
import yaml from 'js-yaml';
import type { KemSchemeYaml, SchemeYaml } from '$lib/types';

interface NgccInstance {
	instance: string;
	ref: string;
	ngcc_level?: number;
	recommended?: boolean;
	omit?: string;
	pk?: number;
	sig?: number;
	ct?: number;
	sk?: number;
	size_ref?: string;
}
interface NgccCandidate {
	id: string;
	kind: 'signature' | 'kem';
	name: string;
	file: string;
	website: string;
	category: string;
	assumption: string;
	spec: string;
	instances: NgccInstance[];
}

const DATA = join(process.cwd(), 'data');
const candidates = yaml.load(readFileSync(join(DATA, 'ngcc', 'candidates.yaml'), 'utf8')) as NgccCandidate[];
const FAMILIES = ['Lattices', 'Code-based', 'Multivariate', 'Symmetric', 'MPCitH', 'Isogenies', 'Other'];
const LEVELS = [128, 256, 384, 512];
const ids = (prefix: string, n: number) => Array.from({ length: n }, (_, i) => `${prefix}-${String(i + 1).padStart(2, '0')}`);
const yamlPath = (c: NgccCandidate) => join(DATA, c.kind === 'signature' ? 'schemes' : 'kems', `${c.file}.yaml`);

describe('data/ngcc/candidates.yaml', () => {
	it('lists the 34 signature and 41 KEM round-1 candidates once each, in order', () => {
		expect(candidates.filter((c) => c.kind === 'signature').map((c) => c.id)).toEqual(ids('sign', 34));
		expect(candidates.filter((c) => c.kind === 'kem').map((c) => c.id)).toEqual(ids('kem', 41));
	});

	it('gives every candidate a site family, an assumption, the official page and a spec', () => {
		const errors = candidates.flatMap((c) => [
			...(FAMILIES.includes(c.category) ? [] : [`${c.id}: unknown family "${c.category}"`]),
			...(c.assumption?.trim() ? [] : [`${c.id}: empty assumption`]),
			...(/^https:\/\/www\.niccs\.org\.cn\//.test(c.website ?? '') ? [] : [`${c.id}: website must be the official NICCS page`]),
			...(c.spec?.trim() ? [] : [`${c.id}: empty spec`])
		]);
		expect(errors).toEqual([]);
	});

	it('gives every instance a spec reference and an NGCC target or a reason to leave it out', () => {
		const errors = candidates.flatMap((c) =>
			c.instances.flatMap((i) => {
				const e: string[] = [];
				if (!i.ref?.trim()) e.push(`${c.id} ${i.instance}: missing ref`);
				if (i.omit !== undefined) {
					if (!i.omit.trim()) e.push(`${c.id} ${i.instance}: empty omit reason`);
					if (i.ngcc_level !== undefined || i.recommended) e.push(`${c.id} ${i.instance}: omitted instances carry no level or recommendation`);
				} else if (!LEVELS.includes(i.ngcc_level as number)) {
					e.push(`${c.id} ${i.instance}: ngcc_level must be 128, 256, 384 or 512`);
				}
				const sizeKey = c.kind === 'signature' ? 'sig' : 'ct';
				if ((i.pk !== undefined || i[sizeKey] !== undefined) && !(i.pk && i[sizeKey] && i.size_ref?.trim()))
					e.push(`${c.id} ${i.instance}: spec sizes need pk, ${sizeKey} and size_ref together`);
				return e;
			})
		);
		expect(errors).toEqual([]);
	});

	it('lists at least one instance per candidate and each instance once per kind', () => {
		for (const kind of ['signature', 'kem'] as const) {
			const names = candidates.filter((c) => c.kind === kind).flatMap((c) => c.instances.map((i) => i.instance));
			expect(names.length).toBeGreaterThan(0);
			expect(names.filter((n, k) => names.indexOf(n) !== k)).toEqual([]);
		}
		expect(candidates.filter((c) => !c.instances.some((i) => i.omit === undefined)).map((c) => c.id)).toEqual([]);
	});

	it('uses safe, unique file names', () => {
		for (const kind of ['signature', 'kem'] as const) {
			const files = candidates.filter((c) => c.kind === kind).map((c) => c.file);
			expect(files.filter((f) => !/^[A-Za-z0-9][A-Za-z0-9._-]*$/.test(f))).toEqual([]);
			expect(new Set(files).size).toBe(files.length);
		}
	});

	it('has a generated YAML file for every candidate', () => {
		expect(candidates.filter((c) => !existsSync(yamlPath(c))).map((c) => c.file)).toEqual([]);
	});

	it('agrees with the generated YAML files, which never replace another scheme', () => {
		const errors = candidates.flatMap((c) => {
			if (!existsSync(yamlPath(c))) return [];
			const doc = yaml.load(readFileSync(yamlPath(c), 'utf8'));
			const sets: { name: string; ngcc_level?: number; recommended?: boolean }[] | undefined =
				c.kind === 'signature'
					? (doc as SchemeYaml).versions.find((v) => v.programmes?.['china-ngcc'])?.parametersets
					: (doc as KemSchemeYaml).programmes?.['china-ngcc'] ? (doc as KemSchemeYaml).parametersets : undefined;
			if (!sets) return [`${c.file}.yaml exists but is not an NGCC entry`];
			const want = c.instances.filter((i) => i.omit === undefined).map((i) => `${i.instance}:${i.ngcc_level}:${!!i.recommended}`);
			const got = sets.map((p) => `${p.name}:${p.ngcc_level}:${!!p.recommended}`);
			return JSON.stringify(got) === JSON.stringify(want) ? [] : [`${c.file}.yaml parameter sets differ from candidates.yaml`];
		});
		expect(errors).toEqual([]);
	});
});
