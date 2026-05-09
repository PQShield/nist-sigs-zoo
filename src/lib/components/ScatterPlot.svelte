<script lang="ts">
	import { onMount } from 'svelte';
	import { getFilterStore } from '$lib/filterStore';
	import { themeStore } from '$lib/themeStore';
	import type { NistLevel, ParameterSet } from '$lib/types';

	const { filteredRows } = getFilterStore();

	let container = $state<HTMLDivElement | null>(null);
	let viewHandle: { finalize(): void } | null = null;
	let renderSeq = 0;

	const STAR = 'M0,-1L0.224,-0.309L0.951,-0.309L0.363,0.118L0.588,0.809L0,0.382L-0.588,0.809L-0.363,0.118L-0.951,-0.309L-0.224,-0.309Z';

	const CATEGORY_SHAPES: Record<string, string> = {
		Standardized: STAR,
		Lattices: 'square',
		'Code-based': 'cross',
		'MPC-in-the-Head': 'diamond',
		Multivariate: 'triangle-up',
		Symmetric: 'triangle-down',
		Isogenies: 'circle',
		'Pre-Quantum': 'triangle-left',
		Other: 'triangle-right'
	};

	function shapeKey(d: ParameterSet): string {
		if (d.status === 'FIPS' || d.status === 'To be standardized') return 'Standardized';
		return d.category;
	}

	function securityStatus(d: ParameterSet): string {
		if (d.broken && !d.classical) return 'broken';
		if (d.warning) return 'warning';
		return 'ok';
	}

	function levelLabel(level: NistLevel): string {
		if (level === 'Pre-Quantum') return 'Pre-Quantum';
		return `Level ${level}`;
	}

	async function render() {
		if (!container) return;
		const seq = ++renderSeq;

		const { default: embed } = await import('vega-embed');
		const data = $filteredRows;
		const isDark = document.documentElement.classList.contains('dark');

		const values = data.map((d) => {
			const notes = [
				d.broken && !d.classical ? `⚠ ${d.broken}` : null,
				d.warning ? `⚠ ${d.warning}` : null
			]
				.filter(Boolean)
				.join(' | ');

			const tooltip: Record<string, string | number> = {
				Scheme: d.scheme,
				'Parameter set': d.parameterset,
				...(d.version ? { Version: d.version } : {}),
				Family: d.category,
				'Security level': levelLabel(d.level),
				'pk (bytes)': d.pk.toLocaleString(),
				'sig (bytes)': d.sig.toLocaleString(),
				...(notes ? { Notes: notes } : {})
			};

			return {
				scheme: d.scheme,
				parameterset: d.parameterset,
				pk: d.pk,
				sig: d.sig,
				shapeKey: shapeKey(d),
				level: levelLabel(d.level),
				security: securityStatus(d),
				tooltip
			};
		});

		// Colors per NIST security level — only levels present in data
		const allLevelDomain = ['Pre-Quantum', 'Level 1', 'Level 2', 'Level 3', 'Level 4', 'Level 5'];
		const allLevelColors = isDark
			? ['#9CA3AF', '#60A5FA', '#34D399', '#FBBF24', '#F97316', '#F87171']
			: ['#6B7280', '#1D4ED8', '#059669', '#D97706', '#EA580C', '#DC2626'];
		const presentLevels = new Set(data.map((d) => levelLabel(d.level)));
		const levelDomain = allLevelDomain.filter((l) => presentLevels.has(l));
		const levelColorRange = allLevelDomain
			.map((l, i) => (presentLevels.has(l) ? allLevelColors[i] : null))
			.filter(Boolean);

		// Shape per shapeKey — ordered by CATEGORY_SHAPES key order, only present in data
		const presentShapeKeys = Object.keys(CATEGORY_SHAPES).filter((k) =>
			values.some((v) => v.shapeKey === k)
		);
		const categoryDomain = presentShapeKeys;
		const categoryShapeRange = presentShapeKeys.map((k) => CATEGORY_SHAPES[k]);

		const legendConfig = {
			labelColor: isDark ? '#94A3B8' : '#475569',
			titleColor: isDark ? '#C5CADA' : '#061128',
			strokeColor: 'transparent',
			padding: 8
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
							bind: 'scales'
						}
					],
					mark: { type: 'point', filled: true, size: 100 },
					encoding: {
						size: {
							condition: [
								{ test: "datum.shapeKey === 'Standardized'", value: 180 },
								{ test: "datum.shapeKey === 'Lattices'", value: 70 }
							],
							value: 100
						},
						color: {
							field: 'level',
							type: 'nominal',
							scale: { domain: levelDomain, range: levelColorRange },
							legend: { title: 'Security level', ...legendConfig }
						},
						shape: {
							field: 'shapeKey',
							type: 'nominal',
							scale: { domain: categoryDomain, range: categoryShapeRange },
							legend: { title: 'Family', ...legendConfig }
						}
					}
				},
				{
					transform: [{ filter: "datum.security === 'broken'" }],
					mark: { type: 'point', filled: false, shape: 'circle', size: 120, strokeWidth: 2 },
					encoding: {
						color: { value: isDark ? '#FCA5A5' : '#B91C1C' }
					}
				},
				{
					transform: [{ filter: "datum.security === 'warning'" }],
					mark: { type: 'point', filled: false, shape: 'circle', size: 120, strokeWidth: 2 },
					encoding: {
						color: { value: isDark ? '#FDE68A' : '#B45309' }
					}
				}
			],
			encoding: {
				x: {
					field: 'pk',
					type: 'quantitative',
					scale: { type: 'log' },
					axis: { title: 'Public key size (bytes)', grid: true, format: 's' }
				},
				y: {
					field: 'sig',
					type: 'quantitative',
					scale: { type: 'log' },
					axis: { title: 'Signature size (bytes)', grid: true, format: 's' }
				},
				tooltip: { field: 'tooltip', type: 'nominal' }
			},
			config: {
				background: 'transparent',
				axis: {
					gridColor: isDark ? '#334155' : '#E2E8F0',
					tickColor: isDark ? '#475569' : '#94A3B8',
					labelColor: isDark ? '#94A3B8' : '#475569',
					titleColor: isDark ? '#C5CADA' : '#061128',
					domainColor: isDark ? '#475569' : '#94A3B8'
				},
				view: { stroke: 'transparent' }
			}
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
		render();
	});
</script>

<div bind:this={container} class="w-full"></div>
<div class="mt-1 flex items-center justify-between">
	<p class="text-xs text-pqs-bluegray dark:text-pqs-steel">Scroll to zoom · drag to pan</p>
	<button
		onclick={render}
		class="text-xs text-pqs-bluegray hover:text-pqs-apricot dark:text-pqs-steel dark:hover:text-pqs-apricot transition-colors"
	>Reset zoom</button>
</div>
