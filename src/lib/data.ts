import { CPUSPEED } from './constants';
import type { DataRanges, NistLevel, ParameterSet, Scheme } from './types';

function parseCsv(text: string): Record<string, string>[] {
	const lines = text.trim().split('\n');
	const headers = splitCsvLine(lines[0]);
	return lines.slice(1).map((line) => {
		const values = splitCsvLine(line);
		return Object.fromEntries(headers.map((h, i) => [h, values[i] ?? '']));
	});
}

function splitCsvLine(line: string): string[] {
	const result: string[] = [];
	let current = '';
	let inQuotes = false;
	for (let i = 0; i < line.length; i++) {
		const ch = line[i];
		if (ch === '"') {
			if (inQuotes && line[i + 1] === '"') {
				current += '"';
				i++;
			} else {
				inQuotes = !inQuotes;
			}
		} else if (ch === ',' && !inQuotes) {
			result.push(current.trim());
			current = '';
		} else {
			current += ch;
		}
	}
	result.push(current.trim());
	return result;
}

function parseNum(s: string): number {
	return parseFloat(s.replace(/,/g, '')) || 0;
}

export function parseSchemes(csv: string): Scheme[] {
	return parseCsv(csv).map((d) => ({
		scheme: d['Scheme'],
		status: d['NIST status'],
		website: d['Website'],
		category: d['Category'],
		assumption: d['Assumption'],
		broken: d['Broken'] === '' ? false : d['Broken'],
		warning: d['Warning'] === '' ? false : d['Warning'],
		info: d['Info'] === '' ? false : d['Info'],
		classical: d['Broken'] === 'classical',
	}));
}

export function parseParameterSets(
	csv: string,
	schemes: Scheme[]
): { rows: ParameterSet[]; ranges: DataRanges } {
	const schemeMap = new Map(schemes.map((s) => [s.scheme, s]));

	const rows: ParameterSet[] = parseCsv(csv).map((d) => {
		const rawSignCycles = parseNum(d['signing (cycles)']);
		const rawVerifyCycles = parseNum(d['verification (cycles)']);
		const signingMs = parseNum(d['signing (ms)']) || null;
		const verificationMs = parseNum(d['verification (ms)']) || null;

		let signingCycles: number;
		let verificationCycles: number;
		let extrapolated: boolean;

		if (rawSignCycles > 0) {
			extrapolated = false;
			signingCycles = rawSignCycles;
			verificationCycles = rawVerifyCycles;
		} else {
			extrapolated = true;
			signingCycles = signingMs != null ? Math.round((CPUSPEED * signingMs) / 1000) : 0;
			verificationCycles =
				verificationMs != null ? Math.round((CPUSPEED * verificationMs) / 1000) : 0;
		}

		const scheme = schemeMap.get(d['Scheme']);
		if (!scheme) {
			throw new Error(`Unknown scheme: ${d['Scheme']}`);
		}

		const rawLevel = d['Security level'];
		const level: NistLevel = rawLevel === 'Pre-Quantum' ? 'Pre-Quantum' : (Number(rawLevel) as 1 | 2 | 3 | 4 | 5);

		const pk = parseNum(d['pk size']);
		const sig = parseNum(d['sig size']);

		return {
			scheme: d['Scheme'],
			parameterset: d['Parameterset'],
			category: scheme.category,
			status: scheme.status,
			level,
			pk,
			sig,
			pkPlusSig: pk + sig,
			signingCycles,
			verificationCycles,
			signingMs,
			verificationMs,
			extrapolated,
			broken: scheme.broken,
			warning: scheme.warning,
			info: scheme.info,
			classical: scheme.classical,
			website: scheme.website,
			assumption: scheme.assumption,
		};
	});

	const ranges: DataRanges = {
		pk: [Math.min(...rows.map((r) => r.pk)), Math.max(...rows.map((r) => r.pk))],
		sig: [Math.min(...rows.map((r) => r.sig)), Math.max(...rows.map((r) => r.sig))],
		pkPlusSig: [
			Math.min(...rows.map((r) => r.pkPlusSig)),
			Math.max(...rows.map((r) => r.pkPlusSig)),
		],
		signingCycles: [
			Math.min(...rows.filter((r) => r.signingCycles > 0).map((r) => r.signingCycles)),
			Math.max(...rows.map((r) => r.signingCycles)),
		],
		verificationCycles: [
			Math.min(
				...rows.filter((r) => r.verificationCycles > 0).map((r) => r.verificationCycles)
			),
			Math.max(...rows.map((r) => r.verificationCycles)),
		],
	};

	return { rows, ranges };
}
