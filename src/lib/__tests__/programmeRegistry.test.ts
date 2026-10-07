import { describe, it, expect } from 'vitest';
import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import yaml from 'js-yaml';
import type { ProgrammeYaml } from '$lib/types';

const programmes = yaml.load(
	readFileSync(join(process.cwd(), 'data', 'programmes.yaml'), 'utf8')
) as ProgrammeYaml[];
const byId = new Map(programmes.map((p) => [p.id, p]));
const DATE = /^\d{4}(-\d{2}(-\d{2})?)?$/;

describe('programme page fields in data/programmes.yaml', () => {
	it('gives every programme a nav label, description, candidates heading and timeline', () => {
		for (const p of programmes) {
			expect(p.nav_label, p.id).toBeTruthy();
			expect(p.description, p.id).toBeTruthy();
			expect(p.candidates_heading, p.id).toBeTruthy();
			expect(p.timeline?.length, p.id).toBeGreaterThan(0);
		}
	});

	it('uses ISO dates (year, month or day precision) in non-decreasing order', () => {
		for (const p of programmes) {
			const dates = (p.timeline ?? []).flatMap((e) => (e.date ? [e.date] : []));
			for (const d of dates) expect(d, `${p.id}: ${d}`).toMatch(DATE);
			expect(dates, p.id).toEqual([...dates].sort());
		}
	});

	it('marks at most one timeline entry as current', () => {
		for (const p of programmes) {
			expect((p.timeline ?? []).filter((e) => e.current).length, p.id).toBeLessThanOrEqual(1);
		}
	});

	it('puts the current marker on the last dated entry', () => {
		for (const p of programmes) {
			const tl = p.timeline ?? [];
			const i = tl.findIndex((e) => e.current);
			if (i < 0) continue;
			expect(tl.slice(i + 1).filter((e) => e.date), p.id).toEqual([]);
		}
	});

	it('links only to https URLs', () => {
		for (const p of programmes) {
			for (const l of p.links ?? []) expect(l.url, `${p.id}: ${l.label}`).toMatch(/^https:\/\//);
			if (p.findings) expect(p.findings.url, p.id).toMatch(/^https:\/\//);
		}
	});

	it('has one cell per column in every security-level row', () => {
		for (const p of programmes) {
			const t = p.security_levels;
			if (!t) continue;
			for (const row of t.rows) expect(row.length, `${p.id}: ${row[0]}`).toBe(t.columns.length);
		}
	});

	it('only gives official counts for primitives in scope', () => {
		for (const p of programmes) {
			for (const kind of Object.keys(p.official_counts ?? {})) {
				expect(p.primitives, `${p.id}: ${kind}`).toContain(kind);
			}
		}
	});

	it('lists the four China NGCC targets, with 384-bit optional', () => {
		const t = byId.get('china-ngcc')?.security_levels;
		expect(t?.rows.map((r) => r[0])).toEqual(['128-bit', '256-bit', '384-bit', '512-bit']);
		expect(t?.rows.map((r) => r[3])).toEqual(['Required', 'Required', 'Optional', 'Required']);
	});

	it('records the China NGCC round-1 dates', () => {
		const dated = Object.fromEntries(
			(byId.get('china-ngcc')?.timeline ?? []).flatMap((e) => (e.date ? [[e.label, e.date]] : []))
		);
		expect(dated).toEqual({
			'Programme launched': '2025-02-05',
			'Call for proposals': '2025-10-09',
			'Submissions closed': '2026-06-30',
			'Round 1': '2026-09-20'
		});
		expect(byId.get('china-ngcc')?.official_counts).toEqual({ signature: 34, kem: 41 });
	});
});
