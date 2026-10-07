import { describe, expect, it } from 'vitest';
import { kemAxisValue, omittedNote, timeTooltip, partitionPlottable, sigAxisValue } from '$lib/chartData';
import { processYamlSchemes } from '$lib/data';
import { processKemSchemes } from '$lib/kemData';
import { CYCLES_PER_US } from '$lib/constants';
import type { SchemeYaml } from '$lib/types';

const SIGS: SchemeYaml[] = [
	{ name: 'Fast', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v', date: '2025-01-01', status: 'On-ramp', parametersets: [{ name: 'f', level: 1, pk: 1, sig: 2, signing_cycles: 2500, verification_cycles: 5000 }] }] },
	{ name: 'Sized', website: '', category: 'Lattices', assumption: '', versions: [{ version: 'v', date: '2026-09-20', status: 'NGCC', parametersets: [{ name: 's', ngcc_level: 128, pk: 3, sig: 4 }] }] }
];
const rows = processYamlSchemes(SIGS).parameterSets;

describe('sigAxisValue', () => {
	it('returns sizes, and displayed times extrapolated from cycles', () => {
		expect(sigAxisValue(rows[0], 'pk')).toBe(1);
		expect(sigAxisValue(rows[0], 'signingTime')).toBe(2500 / CYCLES_PER_US);
		expect(sigAxisValue(rows[0], 'verificationTime')).toBe(5000 / CYCLES_PER_US);
	});
	it('treats a missing benchmark as no value', () => {
		expect(sigAxisValue(rows[1], 'signingTime')).toBeNull();
		expect(sigAxisValue(rows[1], 'signingCycles')).toBeNull();
	});
});

describe('kemAxisValue', () => {
	it('uses measured µs, else cycles', () => {
		const [k] = processKemSchemes([{ name: 'K', website: '', category: 'Lattices', assumption: '', status: 'x',
			parametersets: [{ name: 'k', level: 1, pk: 1, ct: 1, encaps_us: 7, decaps_cycles: 2500 }] }]).parameterSets;
		expect(kemAxisValue(k, 'encapsTime')).toBe(7);
		expect(kemAxisValue(k, 'decapsTime')).toBe(1);
		expect(kemAxisValue(k, 'keygenTime')).toBeNull();
		expect(kemAxisValue(k, 'ct')).toBe(1);
	});
});

describe('partitionPlottable', () => {
	it('leaves out rows without a value on either axis in the Speed view', () => {
		const { plotted, omitted } = partitionPlottable(rows, sigAxisValue, 'signingTime', 'verificationTime');
		expect(plotted.map((r) => r.scheme)).toEqual(['Fast']);
		expect(omitted.map((r) => r.scheme)).toEqual(['Sized']);
	});
	it('plots every row in the Sizes view', () => {
		expect(partitionPlottable(rows, sigAxisValue, 'pk', 'sig').omitted).toEqual([]);
	});
	it('copes with nothing plottable', () => {
		const { plotted, omitted } = partitionPlottable([rows[1]], sigAxisValue, 'signingTime', 'verificationTime');
		expect(plotted).toEqual([]);
		expect(omitted).toHaveLength(1);
	});
});

describe('omittedNote', () => {
	it('says how many parameter sets are missing and why', () => {
		expect(omittedNote(1)).toBe('1 parameter set is not shown: not yet benchmarked.');
		expect(omittedNote(4)).toBe('4 parameter sets are not shown: not yet benchmarked.');
	});
});

describe('timeTooltip', () => {
	it('shows measured times plainly, extrapolated ones marked, and skips missing', () => {
		const t = timeTooltip([
			{ label: 'Signing', us: 12.34, cycles: 0 },
			{ label: 'Verification', us: null, cycles: 5000 },
			{ label: 'Keygen', us: null, cycles: 0 }
		]);
		expect(t['Signing time']).toBe('12.3 µs');
		expect(t['Verification time']).toBe('2.0 µs (estimated from cycles)');
		expect(t).not.toHaveProperty('Keygen time');
	});
});
