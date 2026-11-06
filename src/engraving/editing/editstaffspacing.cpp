/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: staff spacing by hand, see editstaffspacing.h
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "editstaffspacing.h"

#include <algorithm>
#include <vector>

#include "../dom/box.h"
#include "../dom/factory.h"
#include "../dom/measure.h"
#include "../dom/page.h"
#include "../dom/part.h"
#include "../dom/score.h"
#include "../dom/segment.h"
#include "../dom/spacer.h"
#include "../dom/staff.h"
#include "../dom/system.h"
#include "../rendering/score/horizontalspacing.h"

namespace mu::engraving::makam {
// width of the handle zone left of a staff, in spatium
static constexpr double HANDLE_WIDTH = 5.0;

static bool hasStaves(const System* system)
{
    return system && system->firstMeasure() && !system->staves().empty();
}

// The gap above staffIdx of systems[systemIdx], all on one page
static StaffGap gapAbove(const Score* score, const std::vector<System*>& systems, size_t systemIdx, staff_idx_t staffIdx)
{
    System* system = systems.at(systemIdx);
    System* above = system;
    staff_idx_t aboveIdx = system->prevVisibleStaff(staffIdx);
    if (aboveIdx == muse::nidx) {
        // the first staff: the system above on the page, unless it is a frame (title)
        if (systemIdx == 0 || !hasStaves(systems.at(systemIdx - 1))) {
            return {};
        }
        above = systems.at(systemIdx - 1);
        aboveIdx = above->lastVisibleStaff();
        if (aboveIdx == muse::nidx) {
            return {};
        }
    }

    StaffGap g;
    g.measure = system->firstMeasure();
    g.staffIdx = staffIdx;
    g.aboveMeasure = above->firstMeasure();
    g.aboveStaffIdx = aboveIdx;
    const double bottom = above->staffYpage(aboveIdx) + score->staff(aboveIdx)->staffHeight(g.aboveMeasure->tick());
    g.gap = (system->staffYpage(staffIdx) - bottom) / score->style().spatium();
    return g;
}

StaffGap staffGapAt(Score* score, const muse::PointF& pos)
{
    if (!score) {
        return {};
    }
    const Page* page = score->searchPage(pos);
    if (!page) {
        return {};
    }

    const muse::PointF p = pos - page->pos();
    const double sp = score->style().spatium();
    const std::vector<System*>& systems = page->systems();
    for (size_t i = 0; i < systems.size(); ++i) {
        const System* system = systems[i];
        if (!hasStaves(system)) {
            continue;
        }
        const double left = system->x();
        if (p.x() < left - HANDLE_WIDTH * sp || p.x() > left + system->leftMargin()) {
            continue;
        }
        for (staff_idx_t idx = system->firstVisibleStaff(); idx != muse::nidx; idx = system->nextVisibleStaff(idx)) {
            const double top = system->staffYpage(idx);
            const double height = score->staff(idx)->staffHeight(system->firstMeasure()->tick());
            if (p.y() >= top - 0.5 * sp && p.y() <= top + height + 0.5 * sp) {
                return gapAbove(score, systems, i, idx);
            }
        }
    }
    return {};
}

StaffGap staffGapOfSelection(Score* score)
{
    if (!score) {
        return {};
    }
    const Selection& sel = score->selection();
    const Measure* measure = nullptr;
    staff_idx_t staffIdx = muse::nidx;
    if (sel.isRange()) {
        measure = sel.startSegment() ? sel.startSegment()->measure() : nullptr;
        staffIdx = sel.staffStart();
    } else if (!sel.elements().empty()) {
        EngravingItem* e = sel.elements().front();
        measure = e->findMeasure();
        staffIdx = e->staffIdx();
    }

    const System* system = measure ? measure->system() : nullptr;
    const Page* page = system ? system->page() : nullptr;
    if (!page || !hasStaves(system) || staffIdx >= system->staves().size()) {
        return {};
    }
    const std::vector<System*>& systems = page->systems();
    const auto it = std::find(systems.begin(), systems.end(), system);
    if (it == systems.end()) {
        return {};
    }
    return gapAbove(score, systems, static_cast<size_t>(it - systems.begin()), staffIdx);
}

void setStaffGap(Score* score, const StaffGap& gap, double delta, bool thisSystemOnly)
{
    if (!score || !gap.isValid()) {
        return;
    }

    if (score->style().styleB(Sid::enableVerticalSpread)) {
        score->undoChangeStyleVal(Sid::enableVerticalSpread, false);
    }

    const double target = std::max(0.0, gap.gap + delta);

    if (thisSystemOnly) {
        Measure* measure = gap.aboveMeasure;
        Spacer* old = measure->vspacerDown(gap.aboveStaffIdx);
        if (old && old->spacerType() == SpacerType::FIXED) {
            old->undoChangeProperty(Pid::SPACE, Spatium(target));
        } else {
            if (old) {
                score->undoRemoveElement(old);
            }
            Spacer* spacer = Factory::createSpacer(measure);
            spacer->setSpacerType(SpacerType::FIXED);
            spacer->setTrack(gap.aboveStaffIdx * VOICES);
            spacer->setGap(Spatium(target));
            spacer->setParent(measure);
            score->undoAddElement(spacer);
        }
    } else if (gap.betweenSystems()) {
        score->undoChangeStyleVal(Sid::minSystemDistance, Spatium(target));
    } else {
        // the gap is the staff distance of the style plus the distance above the staff
        Staff* staff = score->staff(gap.staffIdx);
        const Staff* above = score->staff(gap.aboveStaffIdx);
        const Sid base = staff->part() == above->part() ? Sid::akkoladeDistance : Sid::staffDistance;
        staff->undoChangeProperty(Pid::STAFF_USERDIST, Spatium(target - score->style().styleS(base).val()));
    }

    score->setLayoutAll();
}

//---------------------------------------------------------
//   Line end
//---------------------------------------------------------

Measure* lineEndAt(Score* score, const muse::PointF& pos)
{
    if (!score) {
        return nullptr;
    }
    const Page* page = score->searchPage(pos);
    if (!page) {
        return nullptr;
    }

    const muse::PointF p = pos - page->pos();
    const double sp = score->style().spatium();
    for (const System* system : page->systems()) {
        if (!hasStaves(system)) {
            continue;
        }
        Measure* last = system->lastMeasure();
        const staff_idx_t first = system->firstVisibleStaff();
        const staff_idx_t lastStaff = system->lastVisibleStaff();
        if (!last || first == muse::nidx || lastStaff == muse::nidx) {
            continue;
        }
        // right of the end barline, so that the barline can still be clicked; over the whole frame
        // that ends the line, so that the frame's own width grip (which works the other way
        // round) is not reached by mistake
        const double right = system->x() + last->x() + last->width();
        double end = right + HANDLE_WIDTH * sp;
        const MeasureBase* next = last->next();
        if (next && next->isHBox() && next->system() == system) {
            end = std::max(end, system->x() + next->x() + next->width() + sp);
        }
        if (p.x() < right + 0.3 * sp || p.x() > end) {
            continue;
        }
        const double top = system->staffYpage(first);
        const double bottom = system->staffYpage(lastStaff) + score->staff(lastStaff)->staffHeight(last->tick());
        if (p.y() >= top - 0.5 * sp && p.y() <= bottom + 0.5 * sp) {
            return last;
        }
    }
    return nullptr;
}

static HBox* frameAfter(const Measure* measure)
{
    MeasureBase* next = measure ? measure->next() : nullptr;
    return next && next->isHBox() ? toHBox(next) : nullptr;
}

bool hasLineEndFrame(const Measure* measure)
{
    return frameAfter(measure) != nullptr;
}

double lineEndFrameWidth(const Measure* measure)
{
    const HBox* frame = frameAfter(measure);
    return frame ? frame->boxWidth().val() : 0.0;
}

double naturalLineEndFrameWidth(Measure* measure)
{
    System* system = measure ? measure->system() : nullptr;
    if (!system || system->measures().empty()) {
        return 0.0;
    }
    using rendering::score::HorizontalSpacing;

    // the spacing without justification puts the measures at their natural x and width
    const MeasureBase* last = system->measures().back();
    const double oldEnd = last->x() + last->width();
    const double target = system->width();
    double width = HorizontalSpacing::computeSpacingForFullSystem(system);
    const double naturalEnd = measure->x() + measure->width();

    // then the line is spaced again as the layout left it (SystemLayout::collectSystem): squeezed,
    // stretched to the page width, or at its natural width
    if (width > target) {
        HorizontalSpacing::squeezeSystemToFit(system, width, target);
    } else if (oldEnd > width + 0.5 * measure->spatium()) {
        HorizontalSpacing::justifySystem(system, width, target);
    }

    // half a space to spare: a line wider than the page would push its last measure to the next line
    return std::max(0.0, (target - naturalEnd) / measure->spatium() - 0.5);
}

double lineEndRight(const Measure* measure)
{
    const System* system = measure ? measure->system() : nullptr;
    return system ? system->canvasPos().x() + system->width() : 0.0;
}

void setLineEnd(Score* score, Measure* measure, double frameWidth)
{
    if (!score || !measure) {
        return;
    }

    HBox* frame = frameAfter(measure);
    if (!frame) {
        frame = toHBox(score->insertBox(ElementType::HBOX, measure->next()));
    }
    frame->undoChangeProperty(Pid::BOX_WIDTH, Spatium(std::max(0.0, frameWidth)));

    // the line break moves from the measure to the frame
    if (!frame->lineBreak()) {
        frame->undoSetBreak(true, LayoutBreakType::LINE);
    }
    if (measure->lineBreak()) {
        measure->undoSetBreak(false, LayoutBreakType::LINE);
    }

    score->setLayoutAll();
}

void removeLineEnd(Score* score, Measure* measure)
{
    HBox* frame = frameAfter(measure);
    if (!score || !frame) {
        return;
    }

    // the line break goes back to the measure, so the line keeps its measures
    if (frame->lineBreak() && !measure->lineBreak()) {
        measure->undoSetBreak(true, LayoutBreakType::LINE);
    }
    score->deleteMeasures(frame, frame);
    score->setLayoutAll();
}

bool dropLineEnd(Score* score, const muse::PointF& pos)
{
    staff_idx_t staffIdx = 0;
    MeasureBase* mb = score ? score->pos2measure(pos, &staffIdx, 0, nullptr, nullptr) : nullptr;
    if (!mb || !mb->isMeasure()) {
        return false;
    }
    Measure* measure = toMeasure(mb);
    setLineEnd(score, measure, naturalLineEndFrameWidth(measure));
    return true;
}
}
