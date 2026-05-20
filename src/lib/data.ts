import { CPUSPEED } from './constants';
import type { DataRanges, NistLevel, ParameterSet, Scheme, SchemeYaml } from './types';

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
		tags: [],
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
			notes: null,
			version: '',
			perfSource: null,
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

export function processYamlSchemes(
	schemeData: SchemeYaml[],
	tagFilter?: string,
	{ useLatestVersion = false }: { useLatestVersion?: boolean } = {}
): {
	schemes: Scheme[];
	parameterSets: ParameterSet[];
	ranges: DataRanges;
} {
	const schemes: Scheme[] = [];
	const rows: ParameterSet[] = [];

	for (const yaml of schemeData) {
		if (!yaml.versions || yaml.versions.length === 0) continue;

		const sorted = [...yaml.versions].sort((a, b) => b.date.localeCompare(a.date));

		let latest;
		if (tagFilter) {
			// Pick latest version with the requested tag
			const tagged = sorted.filter((v) => v.tags?.includes(tagFilter));
			if (tagged.length > 0) {
				// useLatestVersion: include scheme if it has the tag, but show newest version
				latest = useLatestVersion ? sorted[0] : tagged[0];
			} else if (yaml.versions.every((v) => !v.tags || v.tags.length === 0)) {
				// Untagged reference scheme — always include
				latest = sorted[0];
			} else {
				continue; // Scheme exists but not in this round
			}
		} else {
			latest = sorted[0];
		}

		const scheme: Scheme = {
			scheme: yaml.name,
			status: latest.status,
			website: yaml.website,
			category: yaml.category,
			assumption: yaml.assumption,
			broken: latest.broken ?? false,
			warning: latest.warning ?? false,
			info: latest.info ?? false,
			classical: latest.broken === 'classical',
			tags: [...new Set(yaml.versions.flatMap((v) => v.tags ?? []))],
		};
		schemes.push(scheme);

		for (const ps of latest.parametersets) {
			const level: NistLevel =
				ps.level === 'Pre-Quantum' ? 'Pre-Quantum' : (ps.level as 1 | 2 | 3 | 4 | 5);

			const signingMs = ps.signing_ms ?? null;
			const verificationMs = ps.verification_ms ?? null;

			let signingCycles: number;
			let verificationCycles: number;
			let extrapolated: boolean;

			if (ps.signing_cycles != null && ps.signing_cycles > 0) {
				extrapolated = false;
				signingCycles = ps.signing_cycles;
				verificationCycles = ps.verification_cycles ?? 0;
			} else {
				extrapolated = true;
				signingCycles = signingMs != null ? Math.round((CPUSPEED * signingMs) / 1000) : 0;
				verificationCycles =
					verificationMs != null ? Math.round((CPUSPEED * verificationMs) / 1000) : 0;
			}

			// Parameterset flags fall back to version-level flags
			const broken = ps.broken ?? latest.broken ?? false;
			const warning = ps.warning ?? latest.warning ?? false;
			const info = ps.info ?? latest.info ?? false;

			const pk = ps.pk;
			const sig = ps.sig;

			rows.push({
				scheme: yaml.name,
				parameterset: ps.name,
				category: yaml.category,
				status: latest.status,
				level,
				pk,
				sig,
				pkPlusSig: pk + sig,
				signingCycles,
				verificationCycles,
				signingMs,
				verificationMs,
				extrapolated,
				broken: broken === false ? false : broken || false,
				warning: warning === false ? false : warning || false,
				info: info === false ? false : info || false,
				classical: broken === 'classical',
				website: yaml.website,
				assumption: yaml.assumption,
				notes: ps.notes ?? null,
				version: latest.version,
				perfSource: latest.perf_source ?? null,
			});
		}
	}

	const nonZeroSign = rows.filter((r) => r.signingCycles > 0);
	const nonZeroVerify = rows.filter((r) => r.verificationCycles > 0);

	const ranges: DataRanges = {
		pk: [Math.min(...rows.map((r) => r.pk)), Math.max(...rows.map((r) => r.pk))],
		sig: [Math.min(...rows.map((r) => r.sig)), Math.max(...rows.map((r) => r.sig))],
		pkPlusSig: [
			Math.min(...rows.map((r) => r.pkPlusSig)),
			Math.max(...rows.map((r) => r.pkPlusSig)),
		],
		signingCycles: [
			Math.min(...nonZeroSign.map((r) => r.signingCycles)),
			Math.max(...nonZeroSign.map((r) => r.signingCycles)),
		],
		verificationCycles: [
			Math.min(...nonZeroVerify.map((r) => r.verificationCycles)),
			Math.max(...nonZeroVerify.map((r) => r.verificationCycles)),
		],
	};

	return { schemes, parameterSets: rows, ranges };
}
