<script lang="ts">
	import { onMount } from 'svelte';
	import { getKemStore } from '$lib/kemStore';
	import { themeStore } from '$lib/themeStore';
	import type { KemAxisField, KemParameterSet, NistLevel, ScaleType } from '$lib/types';

	let {
		xField = 'pk' as KemAxisField,
		yField = 'ct' as KemAxisField,
		xScale = 'log' as ScaleType,
		yScale = 'log' as ScaleType,
	}: { xField?: KemAxisField; yField?: KemAxisField; xScale?: ScaleType; yScale?: ScaleType } = $props();

	const AXIS_TITLES: Record<KemAxisField, string> = {
		pk: 'Public key (bytes)',
		ct: 'Ciphertext (bytes)',
		pkPlusCt: 'pk + ct (bytes)',
		sk: 'Secret key (bytes)',
		keygenCycles: 'Keygen (cycles)',
		encapsCycles: 'Encaps (cycles)',
		decapsCycles: 'Decaps (cycles)',
		keygenUs: 'Keygen (µs)',
		encapsUs: 'Encaps (µs)',
		decapsUs: 'Decaps (µs)',
	};

	const { filteredRows } = getKemStore();

	let container = $state<HTMLDivElement | null>(null);
	let viewHandle: { finalize(): void } | null = null;
	let renderSeq = 0;

	const STAR = 'M0,-1L0.224,-0.309L0.951,-0.309L0.363,0.118L0.588,0.809L0,0.382L-0.588,0.809L-0.363,0.118L-0.951,-0.309L-0.224,-0.309Z';

	const CATEGORY_SHAPES: Record<string, string> = {
		Standardized: STAR,
		Lattices: 'square',
		'Code-based': 'cross',
		'Pre-Quantum': 'triangle-left',
		Other: 'triangle-right',
	};

	function shapeKey(d: KemParameterSet): string {
		if (d.status === 'FIPS' || d.status === 'To be standardized') return 'Standardized';
		return CATEGORY_SHAPES[d.category] ? d.category : 'Other';
	}

	function securityStatus(d: KemParameterSet): string {
		if (d.broken && !d.classical) return 'broken';
		if (d.warning) return 'warning';
		return 'ok';
	}

	function levelLabel(level: NistLevel): string {
		if (level === 'Pre-Quantum') return 'N/A';
		return `Level ${level}`;
	}

	async function render() {
		if (!container) return;
		const seq = ++renderSeq;

		const { default: embed } = await import('vega-embed');
		const data = $filteredRows;
		const isDark = document.documentElement.classList.contains('dark');

		const values = data.map((d) => {
			const tooltip: Record<string, string | number> = {
				Scheme: d.scheme,
				'Parameter set': d.parameterset,
				Family: d.category,
				'Security level': levelLabel(d.level),
				'pk (bytes)': d.pk.toLocaleString(),
				'ct (bytes)': d.ct.toLocaleString(),
				'pk+ct (bytes)': d.pkPlusCt.toLocaleString(),
				...(d.sk != null ? { 'sk (bytes)': d.sk.toLocaleString() } : {}),
				...(d.keygenUs != null ? { 'Keygen (µs)': d.keygenUs.toLocaleString() } : {}),
				...(d.encapsUs != null ? { 'Encaps (µs)': d.encapsUs.toLocaleString() } : {}),
				...(d.decapsUs != null ? { 'Decaps (µs)': d.decapsUs.toLocaleString() } : {}),
			};

			return {
				scheme: d.scheme,
				parameterset: d.parameterset,
				pk: d.pk,
				ct: d.ct,
				pkPlusCt: d.pkPlusCt,
				sk: d.sk,
				keygenCycles: d.keygenCycles > 0 ? d.keygenCycles : null,
				encapsCycles: d.encapsCycles > 0 ? d.encapsCycles : null,
				decapsCycles: d.decapsCycles > 0 ? d.decapsCycles : null,
				keygenUs: d.keygenUs,
				encapsUs: d.encapsUs,
				decapsUs: d.decapsUs,
				shapeKey: shapeKey(d),
				level: levelLabel(d.level),
				security: securityStatus(d),
				tooltip,
			};
		});

		const allLevelDomain = ['N/A', 'Level 1', 'Level 2', 'Level 3', 'Level 4', 'Level 5'];
		const allLevelColors = isDark
			? ['#9CA3AF', '#60A5FA', '#34D399', '#FBBF24', '#F97316', '#F87171']
			: ['#6B7280', '#1D4ED8', '#059669', '#D97706', '#EA580C', '#DC2626'];
		const presentLevels = new Set(data.map((d) => levelLabel(d.level)));
		const levelDomain = allLevelDomain.filter((l) => presentLevels.has(l));
		const levelColorRange = allLevelDomain
			.map((l, i) => (presentLevels.has(l) ? allLevelColors[i] : null))
			.filter(Boolean);

		const presentShapeKeys = Object.keys(CATEGORY_SHAPES).filter((k) =>
			values.some((v) => v.shapeKey === k)
		);
		const categoryDomain = presentShapeKeys;
		const categoryShapeRange = presentShapeKeys.map((k) => CATEGORY_SHAPES[k]);

		const isMobile = window.innerWidth < 640;
		const mobileLegend = isMobile
			? { orient: 'bottom', direction: 'horizontal', columns: 3, labelFontSize: 10 }
			: {};
		const legendConfig = {
			labelColor: isDark ? '#94A3B8' : '#475569',
			titleColor: isDark ? '#C5CADA' : '#061128',
			strokeColor: 'transparent',
			padding: 8,
			...mobileLegend,
		};
		const shapeLegendConfig = {
			...legendConfig,
			symbolFillColor: isDark ? '#94A3B8' : '#475569',
			symbolStrokeColor: isDark ? '#94A3B8' : '#475569',
		};

		// eslint-disable-next-line @typescript-eslint/no-explicit-any
		const spec: any = {
			$schema: 'https://vega.github.io/schema/vega-lite/v6.json',
			width: 'container',
			height: 350,
			autosize: { type: 'fit-x', resize: true },
			data: { values },
			resolve: { scale: { x: 'shared', y: 'shared', color: 'independent', shape: 'independent', opacity: 'independent' } },
			layer: [
				{
					params: [
						{
							name: 'grid',
							select: { type: 'interval', encodings: ['x', 'y'] },
							bind: 'scales',
						},
					],
					mark: { type: 'point', filled: true, size: 120 },
					encoding: {
						size: {
							condition: [{ test: "datum.shapeKey === 'Standardized'", value: 200 }],
							value: 120,
						},
						color: {
							field: 'level',
							type: 'nominal',
							scale: { domain: levelDomain, range: levelColorRange },
							legend: { title: 'Security level', ...legendConfig },
						},
						shape: {
							field: 'shapeKey',
							type: 'nominal',
							scale: { domain: categoryDomain, range: categoryShapeRange },
							legend: { title: 'Family', ...shapeLegendConfig },
						},
					},
				},
				{
					transform: [{ filter: "datum.security === 'broken'" }],
					mark: { type: 'point', filled: false, shape: 'circle', size: 140, strokeWidth: 2 },
					encoding: { color: { value: isDark ? '#FCA5A5' : '#B91C1C' } },
				},
				{
					transform: [{ filter: "datum.security === 'warning'" }],
					mark: { type: 'point', filled: false, shape: 'circle', size: 140, strokeWidth: 2 },
					encoding: { color: { value: isDark ? '#FDE68A' : '#B45309' } },
				},
			],
			encoding: {
				x: {
					field: xField,
					type: 'quantitative',
					scale: { type: xScale },
					axis: { title: AXIS_TITLES[xField], grid: true, format: 's' },
				},
				y: {
					field: yField,
					type: 'quantitative',
					scale: { type: yScale },
					axis: { title: AXIS_TITLES[yField], grid: true, format: 's' },
				},
				tooltip: { field: 'tooltip', type: 'nominal' },
			},
			config: {
				background: 'transparent',
				axis: {
					gridColor: isDark ? '#334155' : '#E2E8F0',
					tickColor: isDark ? '#475569' : '#94A3B8',
					labelColor: isDark ? '#94A3B8' : '#475569',
					titleColor: isDark ? '#C5CADA' : '#061128',
					domainColor: isDark ? '#475569' : '#94A3B8',
				},
				view: { stroke: 'transparent' },
			},
		};

		viewHandle?.finalize();
		const result = await embed(container, spec, { actions: false, renderer: 'svg' });
		if (seq !== renderSeq) {
			result.finalize();
			return;
		}
		viewHandle = result;
	}

	onMount(() => {
		return () => viewHandle?.finalize();
	});

	$effect(() => {
		$filteredRows;
		$themeStore;
		xField; yField; xScale; yScale;
		render();
	});
</script>

<div bind:this={container} class="w-full"></div>
<p class="sm:hidden mt-1 text-xs text-pqs-bluegray dark:text-pqs-steel">Shape = family · tap point for details</p>
<div class="mt-1 flex items-center justify-between">
	<p class="text-xs text-pqs-bluegray dark:text-pqs-steel">Scroll to zoom · drag to pan</p>
	<button
		onclick={render}
		class="text-xs text-pqs-bluegray hover:text-pqs-apricot dark:text-pqs-steel dark:hover:text-pqs-apricot transition-colors"
	>Reset zoom</button>
</div>
