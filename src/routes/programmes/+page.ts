import { allKemData } from '#lib/kemSchemeData.ts';
import { allProgrammes } from '#lib/programmeData.ts';
import { programmeCards } from '#lib/programmePage.ts';
import { allSchemeData } from '#lib/schemeData.ts';

export function load() {
	return { cards: programmeCards(allProgrammes, allSchemeData, allKemData) };
}
